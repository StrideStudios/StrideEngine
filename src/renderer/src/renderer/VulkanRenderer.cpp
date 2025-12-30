#include "renderer/VulkanRenderer.h"

#include <cmath>
#include <iostream>
#include <glm/glm.hpp>
#include <tracy/Tracy.hpp>
#include <tracy/TracyVulkan.hpp>

#include "scene/world/Camera.h"
#include "rendercore/VulkanDevice.h"
#include "rendercore/VulkanUtils.h"
#include "vulkan/vk_enum_string_helper.h"
#include "VkBootstrap.h"
#include "engine/Engine.h"

#include "engine/EngineSettings.h"
#include "rendercore/EngineLoader.h"
#include "renderer/EngineTextures.h"
#include "scene/base/Scene.h"
#include "renderer/passes/SpritePass.h"
#include "renderer/Swapchain.h"
#include "engine/Viewport.h"
#include "rendercore/BindlessResources.h"
#include "rendercore/VulkanInstance.h"
#include "renderer/object/ObjectRenderer.h"
#include "SDL3/SDL_vulkan.h"

#define SETTINGS_CATEGORY "Engine"
ADD_COMMAND(bool, UseVsync, true);
#undef SETTINGS_CATEGORY

CVulkanRenderer::FrameData::FrameData(const TFrail<CVulkanDevice>& device, const VkCommandPoolCreateInfo& info) {
	mCommandPool = TUnique<CCommandPool>{device, info};

	// Allocate the default command buffer that we will use for rendering
	mMainCommandBuffer = SCommandBuffer(device, *mCommandPool);

	mTracyContext = TracyVkContext(device->getPhysicalDevice(), device->getDevice(), device->getQueue(EQueueType::GRAPHICS).mQueue, mMainCommandBuffer);
}

CVulkanRenderer::FrameData::~FrameData() {
	TracyVkDestroy(mTracyContext);
	mCommandPool.destroy();
}

CVulkanSurface::CVulkanSurface(SDL_Window* window, const TFrail<CVulkanInstance>& instance) {
	mInstance = instance;
	SDL_Vulkan_CreateSurface(window, mInstance->getInstance(), nullptr, &mVkSurface);
}

CVulkanSurface::~CVulkanSurface() {
	vkb::destroy_surface(mInstance->getInstance(), mVkSurface);
}

CVulkanRenderer::CVulkanRenderer(): mVSync(UseVsync.get()) {}

void CVulkanRenderer::immediateSubmit(std::function<void(SCommandBuffer& cmd)>&& function) {

	const VkDevice device = mDevice->getDevice();

	mUploadContext.lockFor([&](const SUploadContext& upload) {
		SCommandBuffer cmd = upload.mCommandBuffer;

		//begin the command buffer recording. We will use this command buffer exactly once before resetting, so we tell vulkan that
		cmd.begin(false);

		//execute the function
		function(cmd);

		cmd.end();

		const VkSubmitInfo submit = CVulkanInfo::submitInfo(&cmd.cmd);

		VK_CHECK(vkResetFences(device, 1, &upload.mUploadFence->mFence));

		//submit command buffer to the queue and execute it.
		// _uploadFence will now block until the graphic commands finish execution
		VK_CHECK(vkQueueSubmit(mDevice->getQueue(EQueueType::UPLOAD).mQueue, 1, &submit, **upload.mUploadFence));

		VK_CHECK(vkWaitForFences(device, 1, &upload.mUploadFence->mFence, true, 1000000000));

		// reset the command buffers inside the command pool
		VK_CHECK(vkResetCommandPool(device, **upload.mCommandPool, 0));
	});
}

void CVulkanRenderer::init() {
	// Ensure the renderer is only created once
	astsOnce(CVulkanRenderer);

	// Initializes the vkb instance
	mInstance = TShared<CVulkanInstance>{};

	// Create a surface for Device to reference
	mVkSurface = TShared<CVulkanSurface>{CEngine::get()->getViewport()->mWindow, mInstance};

	// Create the vulkan device
	mDevice = TShared<CVulkanDevice>{mInstance, mVkSurface->mVkSurface};

	CBindlessResources::get()->init(mDevice);

	VkCommandPoolCreateInfo uploadCommandPoolInfo = CVulkanInfo::createCommandPoolInfo(mDevice->getQueue(EQueueType::UPLOAD).mFamily);
	//create pool for upload context
	mUploadContext->mCommandPool = TUnique<CCommandPool>{mDevice, uploadCommandPoolInfo};

	//allocate the default command buffer that we will use for the instant commands
	mUploadContext->mCommandBuffer = SCommandBuffer(mDevice, *mUploadContext->mCommandPool);

	VkFenceCreateInfo fenceCreateInfo = CVulkanInfo::createFenceInfo(VK_FENCE_CREATE_SIGNALED_BIT);
	mUploadContext->mUploadFence = TUnique<CFence>{mDevice, fenceCreateInfo};

	{
		// Create a command pool for commands submitted to the graphics queue.
		// We also want the pool to allow for resetting of individual command buffers
		const VkCommandPoolCreateInfo commandPoolInfo = CVulkanInfo::createCommandPoolInfo(
			mDevice->getQueue(EQueueType::GRAPHICS).mFamily, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

		mBuffering.data().resize([&](size_t) {
			return TUnique<FrameData>(mDevice, commandPoolInfo);
		});
	}

	// Initialize Allocator
	mAllocator = TShared<CVulkanAllocator>{asShared()};

	mEngineTextures = TShared<CEngineTextures>{asShared(), mAllocator};

	mSceneBuffer.get(mAllocator)->makeGlobal(mDevice);

	// Load textures and meshes
	CEngineLoader::load(mAllocator);

	// Ensure init is called
	CScene::get();
}

//TODO: members are destroyed in reverse order, so that can be used instead.
void CVulkanRenderer::destroy() {
	CRenderer::destroy();

	CScene::get().destroy();

	//TODO: CEngineLoader self destroy
	for (auto& image : CEngineLoader::getImages()) {
		image.second.destroy();
	}

	for (auto& font : CEngineLoader::getFonts()) {
		font.second.mAtlasImage.destroy();
	}

	mSceneBuffer.destroy();

	mEngineTextures.destroy();

	mAllocator.destroy();

	mBuffering.data().forEach([](size_t, TUnique<FrameData>& ptr) {
		ptr.destroy();
	});

	mUploadContext->mUploadFence.destroy();

	mUploadContext->mCommandPool.destroy();

	CBindlessResources::get().destroy();

	mDevice.destroy();

	mVkSurface.destroy();

	mInstance.destroy();
}

TShared<CSwapchain> CVulkanRenderer::getSwapchain() {
	return mEngineTextures->getSwapchain();
}

void CVulkanRenderer::render(SRendererInfo& info) {

	// Set renderer info allocator
	info.allocator = mAllocator->asWeak();

	if (mVSync != UseVsync.get()) {
		mVSync = UseVsync.get();
		msgs("Reallocating Swapchain to {}", UseVsync.get() ? "enable VSync." : "disable VSync.");
		mEngineTextures->getSwapchain()->setDirty();
	}

	if (info.viewport->isDirty()) {
		mEngineTextures->getSwapchain()->setDirty();
		info.viewport->clean();
	}

	auto swapchainDirtyCheck = [&] {
		// Make sure that the swapchain is not dirty before recreating it
		while (mEngineTextures->getSwapchain()->isDirty()) {
			// Wait for gpu before recreating swapchain
			if (!wait()) continue;

			mEngineTextures->reallocate(info, UseVsync.get());

			for (const auto& pass : getPasses()) {
				pass->update();
			}
		}
	};
	swapchainDirtyCheck();

	// Get command buffer from current frame
	SCommandBuffer cmd = mBuffering.getCurrentFrame().mMainCommandBuffer;

	SSwapchainImage* swapchainImage;

	{
		ZoneScopedN("Begin Frame");

		// Wait for the previous render to stop
		if (!mEngineTextures->getSwapchain()->wait(mDevice, mBuffering.getFrameIndex())) {
			return;
		}

		cmd.begin();

		// Get the current swapchain image
		swapchainImage = mEngineTextures->getSwapchain()->getSwapchainImage(mDevice, mBuffering.getFrameIndex()).get();

		// Reset the current fences, done here so the swapchain acquire doesn't stall the engine
		mEngineTextures->getSwapchain()->reset(mDevice, mBuffering.getFrameIndex());

		// Clear the draw image
		CVulkanUtils::transitionImage(cmd, *mEngineTextures->mDrawImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

		constexpr VkClearColorValue color = { .float32 = {0.0, 0.0, 0.0} };
		constexpr VkImageSubresourceRange imageSubresourceRange { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
		vkCmdClearColorImage(cmd, mEngineTextures->mDrawImage->mImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &color, 1, &imageSubresourceRange);

		// Make the swapchain image into writeable mode before rendering
		CVulkanUtils::transitionImage(cmd, *mEngineTextures->mDrawImage, VK_IMAGE_LAYOUT_GENERAL);

		// Flush previous frame resources
		mAllocator->flushFrameData();
	}

	{
		ZoneScopedN("Render Frame");
		{
			ZoneScopedN("Child Render");
			render(cmd);
		}

		CVulkanUtils::transitionImage(cmd, *mEngineTextures->mDrawImage, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
		CVulkanUtils::transitionImage(cmd, *mEngineTextures->mDepthImage, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);

		VkExtent2D extent {
			CEngine::get()->getViewport()->mExtent.x,
			CEngine::get()->getViewport()->mExtent.y
		};

		CScene::get()->update();

		{
			ZoneScopedN("Update Scene Data");

			// Update Scene Data Buffer
			{

				mSceneData.mScreenSize = Vector2f((float)extent.width, (float)extent.height);
				mSceneData.mInvScreenSize = Vector2f(1.f / (float)extent.width, 1.f / (float)extent.height);

				mSceneData.mViewProj = CScene::get()->mMainCamera->getViewProjectionMatrix();

				//some default lighting parameters
				mSceneData.mAmbientColor = glm::vec4(.1f);
				mSceneData.mSunlightColor = glm::vec4(1.f);
				mSceneData.mSunlightDirection = glm::vec4(0,1,0.5,1.f);
			}

			//write the buffer
			auto* sceneUniformData = static_cast<SceneData*>(mSceneBuffer.get(mAllocator)->getMappedData());
			*sceneUniformData = mSceneData;

			mSceneBuffer.get(mAllocator)->updateGlobal(mDevice);
		}

		if (!getPasses().empty()) {
			ZoneScopedN("Render");

			// Tell all passes that rendering has begun
			for (const auto& pass : getPasses()) {
				pass->begin();
			}

			//TODO: personal ObjectRenderer for passes instead
			// Tell all renderers that rendering has begun

			CObjectRendererRegistry::get()->getObjects().forEach([](const TPair<std::string, const TUnique<CObjectRenderer>&>& pair) {
				pair.obj()->begin();
			});

			info.viewport->set(cmd);

			CPass* previousPass = nullptr;
			for (const auto& pass : getPasses()) {
				ZoneScoped;
				std::string passName = pass->getClass()->getName();
				ZoneName(passName.c_str(), passName.size());

				// Restart rendering if passes have different rendering info
				if (previousPass) {
					if (!pass->hasSameRenderingInfo(previousPass)) {
						vkCmdEndRendering(cmd);
						pass->beginRendering(cmd, info.viewport->mExtent, *mEngineTextures->mDrawImage, *mEngineTextures->mDepthImage);
					}
				} else {
					pass->beginRendering(cmd, info.viewport->mExtent, *mEngineTextures->mDrawImage, *mEngineTextures->mDepthImage);
				}

				pass->render(info, cmd);
				previousPass = pass.get();
			}

			vkCmdEndRendering(cmd);

			// Tell all renderers that rendering has ended
			CObjectRendererRegistry::get()->getObjects().forEach([](const TPair<std::string, const TUnique<CObjectRenderer>&>& pair) {
				pair.obj()->end();
			});

			// Tell all passes that rendering has begun
			for (const auto& pass : getPasses()) {
				pass->end();
			}
		}
	}

	{
		ZoneScopedN("End Frame");

		CVulkanUtils::transitionImage(cmd, *mEngineTextures->mDrawImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

		// Make the swapchain image into presentable mode
		CVulkanUtils::transitionImage(cmd, swapchainImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

		// Execute a copy from the draw image into the swapchain
		auto [width, height, depth] = mEngineTextures->mDrawImage->getExtent();
		CVulkanUtils::copyImageToImage(cmd, mEngineTextures->mDrawImage->mImage, swapchainImage->mImage, {width, height}, mEngineTextures->getSwapchain()->mSwapchain->mInternalSwapchain->extent);

		// Set swapchain image layout to Present so we can show it on the screen
		CVulkanUtils::transitionImage(cmd, swapchainImage, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

		//finalize the command buffer (we can no longer add commands, but it can now be executed)
		cmd.end();

		mEngineTextures->getSwapchain()->submit(cmd, mDevice->getQueue(EQueueType::GRAPHICS).mQueue, mBuffering.getFrameIndex(), swapchainImage->mBindlessAddress);

		//increase the number of frames drawn
		getBufferingType().incrementFrame();

		// Tell tracy we just rendered a frame
		FrameMark;
	}
}

bool CVulkanRenderer::wait() {
	// Make sure the gpu is not working
	vkDeviceWaitIdle(mDevice->getDevice());

	return mEngineTextures->getSwapchain()->wait(mDevice, mBuffering.getFrameIndex());
}

EXPORT TShared<CVulkanDevice> CVulkanRenderer::device() {
	return mDevice;
}

EXPORT TShared<CVulkanInstance> CVulkanRenderer::instance() {
	return mInstance;
}

void CNullRenderer::render(VkCommandBuffer cmd) {
	//CVulkanUtils::transitionImage(cmd, mEngineTextures->mDrawImage->mImage,VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
}

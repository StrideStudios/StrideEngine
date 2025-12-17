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
#include "renderer/EngineUI.h"
#include "renderer/passes/MeshPass.h"
#include "scene/base/Scene.h"
#include "renderer/passes/SpritePass.h"
#include "renderer/Swapchain.h"
#include "engine/Viewport.h"
#include "rendercore/VulkanInstance.h"
#include "renderer/object/ObjectRenderer.h"
#include "SDL3/SDL_vulkan.h"

#define SETTINGS_CATEGORY "Engine"
ADD_COMMAND(bool, UseVsync, true);
#undef SETTINGS_CATEGORY

static CResourceManager gInstanceManager;

CVulkanRenderer* CVulkanRenderer::get() {
	return static_cast<CVulkanRenderer*>(CRenderer::get());
}

CVulkanRenderer::CVulkanRenderer(): mVSync(UseVsync.get()) {}

void CVulkanRenderer::immediateSubmit(std::function<void(SCommandBuffer& cmd)>&& function) {

	CVulkanRenderer& renderer = *get();
	const VkDevice device = CVulkanDevice::vkDevice();

	std::unique_lock lock(renderer.mUploadContext.mMutex);

	SCommandBuffer cmd = renderer.mUploadContext.mCommandBuffer;

	//begin the command buffer recording. We will use this command buffer exactly once before resetting, so we tell vulkan that
	cmd.begin(false);

	//execute the function
	function(cmd);

	cmd.end();

	VkSubmitInfo submit = CVulkanInfo::submitInfo(&cmd.cmd);

	VK_CHECK(vkResetFences(device, 1, &renderer.mUploadContext.mUploadFence->mFence));

	//submit command buffer to the queue and execute it.
	// _uploadFence will now block until the graphic commands finish execution
	VK_CHECK(vkQueueSubmit(CVulkanDevice::get().getQueue(EQueueType::UPLOAD).mQueue, 1, &submit, *renderer.mUploadContext.mUploadFence));

	VK_CHECK(vkWaitForFences(device, 1, &renderer.mUploadContext.mUploadFence->mFence, true, 1000000000));

	// reset the command buffers inside the command pool
	VK_CHECK(vkResetCommandPool(device, *renderer.mUploadContext.mCommandPool, 0));
}

void CVulkanRenderer::init() {
	// Ensure the renderer is only created once
	astsOnce(CVulkanRenderer);

	// Initializes the vkb instance
	//gInstanceManager.create(m_Instance);
	CVulkanInstance& inst = CVulkanInstance::get();

	// Create a surface for Device to reference
	SDL_Vulkan_CreateSurface(CEngine::get().getViewport()->mWindow, inst.getInstance(), nullptr, &mVkSurface);

	CResourceManager::get().callback([&] {
		vkb::destroy_surface(CVulkanInstance::instance(), mVkSurface);//TODO: in instance?
	}); //TODO: not a big fan of the callbacks, should be its own class

	// Create the vulkan device
	//gInstanceManager.create(m_Device, m_Instance, mVkSurface);
	CVulkanDevice& device = CVulkanDevice::get();
	device.init(mVkSurface);

	VkCommandPoolCreateInfo uploadCommandPoolInfo = CVulkanInfo::createCommandPoolInfo(CVulkanDevice::get().getQueue(EQueueType::UPLOAD).mFamily);
	//create pool for upload context
	CResourceManager::get().create(mUploadContext.mCommandPool, uploadCommandPoolInfo);

	//allocate the default command buffer that we will use for the instant commands
	mUploadContext.mCommandBuffer = SCommandBuffer(mUploadContext.mCommandPool);

	VkFenceCreateInfo fenceCreateInfo = CVulkanInfo::createFenceInfo(VK_FENCE_CREATE_SIGNALED_BIT);
	CResourceManager::get().create(mUploadContext.mUploadFence, fenceCreateInfo);

	{
		// Create a command pool for commands submitted to the graphics queue.
		// We also want the pool to allow for resetting of individual command buffers
		const VkCommandPoolCreateInfo commandPoolInfo = CVulkanInfo::createCommandPoolInfo(
			CVulkanDevice::get().getQueue(EQueueType::GRAPHICS).mFamily, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

		mBuffering.forEach([commandPoolInfo](FrameData& frame) {
			CResourceManager::get().create(frame.mCommandPool, commandPoolInfo);

			// Allocate the default command buffer that we will use for rendering
			frame.mMainCommandBuffer = SCommandBuffer(frame.mCommandPool);

			frame.mTracyContext = TracyVkContext(CVulkanDevice::vkPhysicalDevice(), CVulkanDevice::vkDevice(), CVulkanDevice::get().getQueue(EQueueType::GRAPHICS).mQueue, frame.mMainCommandBuffer);
		});

		CResourceManager::get().callback([&] {
			mBuffering.forEach([](const FrameData& frame) {
				TracyVkDestroy(frame.mTracyContext);
			});
		});//TODO: not a big fan of the callbacks, should be its own class
	}

	// Initialize allocator
	CVulkanAllocator::get().init();

	CResourceManager::get().create(mEngineTextures);

	mSceneBuffer.get()->makeGlobal();

	// Load textures and meshes
	CEngineLoader::load();

	//CPassDeferredRegistry::init(CResourceManager::get());
}

void CVulkanRenderer::destroy() {

	mSceneBuffer.destroy();

	gInstanceManager.flush();
}

CSwapchain* CVulkanRenderer::getSwapchain() {
	return mEngineTextures->getSwapchain();
}

void CVulkanRenderer::render(const SRendererInfo& info) {

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

			mEngineTextures->reallocate(UseVsync.get());

			for (const auto pass : getPasses()) {
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
		if (!mEngineTextures->getSwapchain()->wait(mBuffering.getFrameIndex())) {
			return;
		}

		cmd.begin();

		// Get the current swapchain image
		swapchainImage = mEngineTextures->getSwapchain()->getSwapchainImage(mBuffering.getFrameIndex());

		// Reset the current fences, done here so the swapchain acquire doesn't stall the engine
		mEngineTextures->getSwapchain()->reset(mBuffering.getFrameIndex());

		// Clear the draw image
		CVulkanUtils::transitionImage(cmd, mEngineTextures->mDrawImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

		constexpr VkClearColorValue color = { .float32 = {0.0, 0.0, 0.0} };
		constexpr VkImageSubresourceRange imageSubresourceRange { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
		vkCmdClearColorImage(cmd, mEngineTextures->mDrawImage->mImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &color, 1, &imageSubresourceRange);

		// Make the swapchain image into writeable mode before rendering
		CVulkanUtils::transitionImage(cmd, mEngineTextures->mDrawImage, VK_IMAGE_LAYOUT_GENERAL);

		// Flush previous frame resources
		CVulkanAllocator::flushFrameData();
	}

	{
		ZoneScopedN("Render Frame");
		{
			ZoneScopedN("Child Render");
			render(cmd);
		}

		CVulkanUtils::transitionImage(cmd, mEngineTextures->mDrawImage, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
		CVulkanUtils::transitionImage(cmd, mEngineTextures->mDepthImage, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);

		VkExtent2D extent {
			CEngine::get().getViewport()->mExtent.x,
			CEngine::get().getViewport()->mExtent.y
		};

		CScene::get().update();

		{
			ZoneScopedN("Update Scene Data");

			// Update Scene Data Buffer
			{

				mSceneData.mScreenSize = Vector2f((float)extent.width, (float)extent.height);
				mSceneData.mInvScreenSize = Vector2f(1.f / (float)extent.width, 1.f / (float)extent.height);

				mSceneData.mViewProj = CScene::get().mMainCamera->getViewProjectionMatrix();

				//some default lighting parameters
				mSceneData.mAmbientColor = glm::vec4(.1f);
				mSceneData.mSunlightColor = glm::vec4(1.f);
				mSceneData.mSunlightDirection = glm::vec4(0,1,0.5,1.f);

				//allocate a new uniform buffer for the scene data
				//m_GPUSceneDataBuffer = renderer.getCurrentFrame().mFrameResourceManager.allocateBuffer(sizeof(Data), VMA_MEMORY_USAGE_CPU_TO_GPU, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
			}

			//write the buffer
			auto* sceneUniformData = static_cast<SceneData*>(mSceneBuffer.get()->getMappedData());
			*sceneUniformData = mSceneData;

			mSceneBuffer.get()->updateGlobal();
		}

		if (!getPasses().empty()) {
			ZoneScopedN("Render");

			// Tell all passes that rendering has begun
			for (const auto pass : getPasses()) {
				pass->begin();
			}

			//TODO: personal ObjectRenderer for passes instead
			// Tell all renderers that rendering has begun
			CObjectRendererRegistry::forEach([](const std::string& inName, CObjectRenderer*& object) {
				object->begin();
			});

			info.viewport->set(cmd);

			CPass* previousPass = nullptr;
			for (const auto pass : getPasses()) {
				ZoneScoped;
				std::string passName = pass->getClass()->getName();
				ZoneName(passName.c_str(), passName.size());

				// Restart rendering if passes have different rendering info
				if (previousPass) {
					if (!pass->hasSameRenderingInfo(previousPass)) {
						vkCmdEndRendering(cmd);
						pass->beginRendering(cmd, info.viewport->mExtent, mEngineTextures->mDrawImage, mEngineTextures->mDepthImage);
					}
				} else {
					pass->beginRendering(cmd, info.viewport->mExtent, mEngineTextures->mDrawImage, mEngineTextures->mDepthImage);
				}

				pass->render(cmd);
				previousPass = pass;
			}

			vkCmdEndRendering(cmd);

			// Tell all renderers that rendering has ended
			CObjectRendererRegistry::forEach([](const std::string& inName, CObjectRenderer* object) {
				object->end();
			});

			// Tell all passes that rendering has begun
			for (const auto pass : getPasses()) {
				pass->end();
			}
		}
	}

	{
		ZoneScopedN("End Frame");

		CVulkanUtils::transitionImage(cmd, mEngineTextures->mDrawImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

		// Make the swapchain image into presentable mode
		CVulkanUtils::transitionImage(cmd, swapchainImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

		// Execute a copy from the draw image into the swapchain
		auto [width, height, depth] = mEngineTextures->mDrawImage->getExtent();
		CVulkanUtils::copyImageToImage(cmd, mEngineTextures->mDrawImage->mImage, swapchainImage->mImage, {width, height}, mEngineTextures->getSwapchain()->mSwapchain->mInternalSwapchain->extent);

		// Set swapchain image layout to Present so we can show it on the screen
		CVulkanUtils::transitionImage(cmd, swapchainImage, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

		//finalize the command buffer (we can no longer add commands, but it can now be executed)
		cmd.end();

		mEngineTextures->getSwapchain()->submit(cmd, CVulkanDevice::get().getQueue(EQueueType::GRAPHICS).mQueue, mBuffering.getFrameIndex(), swapchainImage->mBindlessAddress);

		//increase the number of frames drawn
		getBufferingType().incrementFrame();

		// Tell tracy we just rendered a frame
		FrameMark;
	}
}

bool CVulkanRenderer::wait() {
	// Make sure the gpu is not working
	vkDeviceWaitIdle(CVulkanDevice::vkDevice());

	return mEngineTextures->getSwapchain()->wait(mBuffering.getFrameIndex());
}

void CNullRenderer::render(VkCommandBuffer cmd) {
	//CVulkanUtils::transitionImage(cmd, mEngineTextures->mDrawImage->mImage,VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
}

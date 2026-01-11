#include "renderer/VulkanRenderer.h"

#include <iostream>
#include <glm/glm.hpp>
#include <tracy/Tracy.hpp>
#include <tracy/TracyVulkan.hpp>

#include "scene/world/Camera.h"
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
#include "VRI/BindlessResources.h"
#include "rendercore/RenderThread.h"
#include "renderer/object/ObjectRenderer.h"
#include "renderer/object/StaticMeshObjectRenderer.h"
#include "SDL3/SDL_vulkan.h"
#include "VRI/VRICommands.h"

#define SETTINGS_CATEGORY "Engine"
ADD_COMMAND(bool, UseVsync, true);
#undef SETTINGS_CATEGORY

CVulkanRenderer::FrameData::FrameData(const VkCommandPoolCreateInfo& info) {
	mCommandPool = TUnique<CCommandPool>{info};

	// Allocate the default command buffer that we will use for rendering
	mCommands = TUnique<CVRICommands>{mCommandPool};

	mTracyContext = TracyVkContext(CVRI::get()->getDevice()->physical_device, CVRI::get()->getDevice()->device, CVRI::get()->getQueue(EQueueType::GRAPHICS).mQueue, mCommands->cmd);
}

CVulkanRenderer::FrameData::~FrameData() {
	TracyVkDestroy(mTracyContext);
	mCommandPool.destroy();
}

CVulkanRenderer::CVulkanRenderer(): mVSync(UseVsync.get()) {}

void CVulkanRenderer::immediateSubmit(std::function<void(TFrail<CVRICommands>)>&& function) {

	mUploadContext.lockFor([&](const SUploadContext& upload) {
		TFrail<CVRICommands> cmd = upload.mCommands;

		//begin the command buffer recording. We will use this command buffer exactly once before resetting, so we tell vulkan that
		cmd->begin(false);

		//execute the function
		function(cmd);

		cmd->end();

		const VkSubmitInfo submit = cmd->submitInfo0();

		VK_CHECK(vkResetFences(CVRI::get()->getDevice()->device, 1, &upload.mUploadFence->mFence));

		//submit command buffer to the queue and execute it.
		// _uploadFence will now block until the graphic commands finish execution
		VK_CHECK(vkQueueSubmit(CVRI::get()->getQueue(EQueueType::UPLOAD).mQueue, 1, &submit, *upload.mUploadFence));

		VK_CHECK(vkWaitForFences(CVRI::get()->getDevice()->device, 1, &upload.mUploadFence->mFence, true, 1000000000));

		// reset the command buffers inside the command pool
		VK_CHECK(vkResetCommandPool(CVRI::get()->getDevice()->device, *upload.mCommandPool, 0));
	});
}

void CVulkanRenderer::init() {
	// Ensure the renderer is only created once
	astsOnce(CVulkanRenderer);

	CVRI::get()->init(CEngine::get()->getViewport()->mWindow);

	CBindlessResources::get()->init();

	VkCommandPoolCreateInfo uploadCommandPoolInfo = CVulkanInfo::createCommandPoolInfo(CVRI::get()->getQueue(EQueueType::UPLOAD).mFamily);
	//create pool for upload context
	mUploadContext->mCommandPool = TUnique<CCommandPool>{uploadCommandPoolInfo};

	//allocate the default command buffer that we will use for the instant commands
	mUploadContext->mCommands = TUnique<CVRICommands>(mUploadContext->mCommandPool);

	VkFenceCreateInfo fenceCreateInfo = CVulkanInfo::createFenceInfo(VK_FENCE_CREATE_SIGNALED_BIT);
	mUploadContext->mUploadFence = TUnique<CFence>{fenceCreateInfo};

	{
		// Create a command pool for commands submitted to the graphics queue.
		// We also want the pool to allow for resetting of individual command buffers
		const VkCommandPoolCreateInfo commandPoolInfo = CVulkanInfo::createCommandPoolInfo(
			CVRI::get()->getQueue(EQueueType::GRAPHICS).mFamily, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

		mBuffering.data().resize([&](size_t) {
			return TUnique<FrameData>(commandPoolInfo);
		});
	}

	mEngineTextures = TShared<CEngineTextures>{this};

	mSceneBuffer.get()->makeGlobal();

	// Load textures and meshes
	CEngineLoader::load(this);
}

//TODO: members are destroyed in reverse order, so that can be used instead.
void CVulkanRenderer::destroy() {
	CRenderer::destroy();

	//TODO: CEngineLoader self destroy
	for (auto& image : CEngineLoader::getImages()) {
		image.second.destroy();
	}

	for (auto& font : CEngineLoader::getFonts()) {
		font.second.mAtlasImage.destroy();
	}

	mSceneBuffer.destroy();

	mEngineTextures.destroy();

	mBuffering.data().forEach([](size_t, TUnique<FrameData>& ptr) {
		ptr.destroy();
	});

	mUploadContext->mUploadFence.destroy();

	mUploadContext->mCommandPool.destroy();

	CBindlessResources::get().destroy();

	CVRI::get()->destroy();
}

TFrail<CSwapchain> CVulkanRenderer::getSwapchain() {
	return mEngineTextures->getSwapchain();
}

void CVulkanRenderer::render(SRendererInfo& info) {
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
	TFrail<CVRICommands> cmd = mBuffering.getCurrentFrame().mCommands;

	SSwapchainImage* swapchainImage;

	{
		ZoneScopedN("Begin Frame");

		// Wait for the previous render to stop
		if (!mEngineTextures->getSwapchain()->wait(mBuffering.getFrameIndex())) {
			return;
		}

		cmd->begin();

		// Get the current swapchain image
		swapchainImage = mEngineTextures->getSwapchain()->getSwapchainImage(mBuffering.getFrameIndex()).get();

		// Reset the current fences, done here so the swapchain acquire doesn't stall the engine
		mEngineTextures->getSwapchain()->reset(mBuffering.getFrameIndex());

		// Clear the draw image
		cmd->transitionImage( mEngineTextures->mDrawImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
		cmd->clearImage(mEngineTextures->mDrawImage);

		// Make the swapchain image into writeable mode before rendering
		cmd->transitionImage( mEngineTextures->mDrawImage, VK_IMAGE_LAYOUT_GENERAL);
	}

	{
		ZoneScopedN("Render Frame");
		{
			ZoneScopedN("Child Render");
			render(cmd);
		}

		cmd->transitionImage(mEngineTextures->mDrawImage, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
		cmd->transitionImage(mEngineTextures->mDepthImage, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);

		VkExtent2D extent {
			CEngine::get()->getViewport()->mExtent.x,
			CEngine::get()->getViewport()->mExtent.y
		};

		info.scene->update();

		{
			ZoneScopedN("Update Scene Data");

			// Update Scene Data Buffer
			{

				mSceneData.mScreenSize = Vector2f((float)extent.width, (float)extent.height);
				mSceneData.mInvScreenSize = Vector2f(1.f / (float)extent.width, 1.f / (float)extent.height);

				mSceneData.mViewProj = info.scene->mMainCamera->getViewProjectionMatrix();

				//some default lighting parameters
				mSceneData.mAmbientColor = glm::vec4(.1f);
				mSceneData.mSunlightColor = glm::vec4(1.f);
				mSceneData.mSunlightDirection = glm::vec4(0,1,0.5,1.f);
			}

			//write the buffer
			auto* sceneUniformData = static_cast<SceneData*>(mSceneBuffer.get()->getMappedData());
			*sceneUniformData = mSceneData;

			mSceneBuffer.get()->updateGlobal();
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

			cmd->setViewportScissor(info.viewport->mExtent);

			CPass* previousPass = nullptr;
			for (const auto& pass : getPasses()) {
				ZoneScoped;
				std::string passName = pass->getClass()->getName();
				ZoneName(passName.c_str(), passName.size());

				// Restart rendering if passes have different rendering info
				if (previousPass) {
					if (!pass->hasSameRenderingInfo(previousPass)) {
						cmd->endRendering();
						pass->beginRendering(cmd, info.viewport->mExtent, mEngineTextures->mDrawImage, mEngineTextures->mDepthImage);
					}
				} else {
					pass->beginRendering(cmd, info.viewport->mExtent, mEngineTextures->mDrawImage, mEngineTextures->mDepthImage);
				}

				pass->render(info, cmd);
				previousPass = pass.get();
			}

			cmd->endRendering();

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

		cmd->transitionImage(mEngineTextures->mDrawImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

		// Make the swapchain image into presentable mode
		cmd->transitionImage(swapchainImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

		// Execute a copy from the draw image into the swapchain
		auto [width, height, depth] = mEngineTextures->mDrawImage->getExtent();
		auto [dstWidth, dstHeight] = mEngineTextures->getSwapchain()->mSwapchain->mInternalSwapchain->extent;
		cmd->blitImage(mEngineTextures->mDrawImage, swapchainImage, {width, height}, {dstWidth, dstHeight});

		// Set swapchain image layout to Present so we can show it on the screen
		cmd->transitionImage(swapchainImage, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

		//finalize the command buffer (we can no longer add commands, but it can now be executed)
		cmd->end();

		mEngineTextures->getSwapchain()->submit(cmd, CVRI::get()->getQueue(EQueueType::GRAPHICS).mQueue, mBuffering.getFrameIndex(), swapchainImage->mBindlessAddress);

		//increase the number of frames drawn
		getBufferingType().incrementFrame();

		// Tell tracy we just rendered a frame
		FrameMark;
	}
}

bool CVulkanRenderer::wait() {
	// Make sure the gpu is not working
	vkDeviceWaitIdle(CVRI::get()->getDevice()->device);

	return mEngineTextures->getSwapchain()->wait(mBuffering.getFrameIndex());
}

void CNullRenderer::render(const TFrail<CVRICommands>& cmd) {
	//CVulkanUtils::transitionImage(cmd, mEngineTextures->mDrawImage->mImage,VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
}

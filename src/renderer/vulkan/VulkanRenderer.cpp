#include "VulkanRenderer.h"

#include <cmath>
#include <iostream>
#include <glm/glm.hpp>
#include <tracy/Tracy.hpp>
#include <tracy/TracyVulkan.hpp>

#include "Camera.h"
#include "VulkanDevice.h"
#include "VulkanUtils.h"
#include "vulkan/vk_enum_string_helper.h"
#include "VkBootstrap.h"

#include "Engine.h"
#include "ShaderCompiler.h"
#include "EngineSettings.h"
#include "EngineTextures.h"
#include "EngineUI.h"
#include "MeshPass.h"
#include "Swapchain.h"

#define SETTINGS_CATEGORY "Engine"
ADD_COMMAND(bool, UseVsync, true);
#undef SETTINGS_CATEGORY

CVulkanRenderer::CVulkanRenderer(): mVSync(UseVsync.get()) {}

void CVulkanRenderer::immediateSubmit(std::function<void(VkCommandBuffer cmd)>&& function) {

	CVulkanRenderer& renderer = CEngine::renderer();
	const VkDevice device = CEngine::device();

	std::unique_lock lock(renderer.mUploadContext.mMutex);

	VkCommandBuffer cmd = renderer.mUploadContext.mCommandBuffer;

	//begin the command buffer recording. We will use this command buffer exactly once before resetting, so we tell vulkan that
	VkCommandBufferBeginInfo cmdBeginInfo = CVulkanInfo::createCommandBufferBeginInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

	VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));

	//execute the function
	function(cmd);

	VK_CHECK(vkEndCommandBuffer(cmd));

	VkSubmitInfo submit = CVulkanInfo::submitInfo(&cmd);

	VK_CHECK(vkResetFences(device, 1, &renderer.mUploadContext.mUploadFence->mFence));

	//submit command buffer to the queue and execute it.
	// _uploadFence will now block until the graphic commands finish execution
	VK_CHECK(vkQueueSubmit(CVulkanDevice::getQueue(EQueueType::UPLOAD).mQueue, 1, &submit, *renderer.mUploadContext.mUploadFence));

	VK_CHECK(vkWaitForFences(device, 1, &renderer.mUploadContext.mUploadFence->mFence, true, 1000000000));

	// reset the command buffers inside the command pool
	VK_CHECK(vkResetCommandPool(device, *renderer.mUploadContext.mCommandPool, 0));
}

void CVulkanRenderer::init() {
	// Ensure the renderer is only created once
	astsOnce(CVulkanRenderer);

	// Initialize the allocator
	CVulkanResourceManager::init();

	VkCommandPoolCreateInfo uploadCommandPoolInfo = CVulkanInfo::createCommandPoolInfo(CVulkanDevice::getQueue(EQueueType::UPLOAD).mFamily);
	//create pool for upload context
	mGlobalResourceManager.createDestroyable(mUploadContext.mCommandPool, uploadCommandPoolInfo);

	//allocate the default command buffer that we will use for the instant commands
	VkCommandBufferAllocateInfo cmdAllocInfo = CVulkanInfo::createCommandAllocateInfo(*mUploadContext.mCommandPool, 1);
	mUploadContext.mCommandBuffer = CVulkanResourceManager::allocateCommandBuffer(cmdAllocInfo);

	VkFenceCreateInfo fenceCreateInfo = CVulkanInfo::createFenceInfo(VK_FENCE_CREATE_SIGNALED_BIT);
	mGlobalResourceManager.createDestroyable(mUploadContext.mUploadFence, fenceCreateInfo);

	{
		// Create a command pool for commands submitted to the graphics queue.
		// We also want the pool to allow for resetting of individual command buffers
		const VkCommandPoolCreateInfo commandPoolInfo = CVulkanInfo::createCommandPoolInfo(
			CVulkanDevice::getQueue(EQueueType::GRAPHICS).mFamily, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

		for (auto &frame: mFrames) {
			mGlobalResourceManager.createDestroyable(frame.mCommandPool, commandPoolInfo);

			// Allocate the default command buffer that we will use for rendering
			VkCommandBufferAllocateInfo frameCmdAllocInfo = CVulkanInfo::createCommandAllocateInfo(*frame.mCommandPool, 1);

			frame.mMainCommandBuffer = CVulkanResourceManager::allocateCommandBuffer(frameCmdAllocInfo);

			frame.mTracyContext = TracyVkContext(CEngine::physicalDevice(), CEngine::device(), CVulkanDevice::getQueue(EQueueType::GRAPHICS).mQueue, frame.mMainCommandBuffer);
		}
	}

	mGlobalResourceManager.createDestroyable(mEngineTextures);

	mSceneDataBuffer = mGlobalResourceManager.allocateGlobalBuffer(sizeof(SceneData), VMA_MEMORY_USAGE_CPU_TO_GPU, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);

	mGlobalResourceManager.createDestroyable(mBasePass, EMeshPass::BASE_PASS);

	mGlobalResourceManager.createDestroyable(mSpritePass);

	// Setup Engine UI
	EngineUI::init(CVulkanDevice::getQueue(EQueueType::GRAPHICS).mQueue, mEngineTextures->getSwapchain().mFormat);
}

void CVulkanRenderer::destroy() {
	EngineUI::destroy();

	for (auto& frame : mFrames) {
		TracyVkDestroy(frame.mTracyContext);

		frame.mFrameResourceManager.flush();
	}

	mGlobalResourceManager.flush();

	// Destroy allocator, if not all allocations have been destroyed, it will throw an error
	CVulkanResourceManager::destroy();
}

void CVulkanRenderer::draw() {
	if (mVSync != UseVsync.get()) {
		mVSync = UseVsync.get();
		msgs("Reallocating Swapchain to {}", UseVsync.get() ? "enable VSync." : "disable VSync.");
		mEngineTextures->getSwapchain().setDirty();
	}

	auto swapchainDirtyCheck = [&] {
		// Make sure that the swapchain is not dirty before recreating it
		while (mEngineTextures->getSwapchain().isDirty()) {
			// Wait for gpu before recreating swapchain
			if (!waitForGpu()) continue;

			mEngineTextures->reallocate(UseVsync.get());
		}
	};
	swapchainDirtyCheck();

	// Get command buffer from current frame
	VkCommandBuffer cmd = getCurrentFrame().mMainCommandBuffer;

	VkImage swapchainImage;
	VkImageView swapchainImageView;
	uint32 swapchainImageIndex;

	{
		ZoneScopedN("Begin Frame");

		EngineUI::begin();

		// Wait for the previous render to stop
		if (!mEngineTextures->getSwapchain().wait(getFrameIndex())) {
			return;
		}

		{
			ZoneScopedN("Begin Command Buffer");

			VK_CHECK(vkResetCommandBuffer(cmd, 0));

			// Tell Vulkan the buffer will only be used once
			VkCommandBufferBeginInfo cmdBeginInfo = CVulkanInfo::createCommandBufferBeginInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
			VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));
		}

		// Get the current swapchain image
		const auto [outSwapchainImage, outSwapchainImageView, outSwapchainImageIndex] = mEngineTextures->getSwapchain().getSwapchainImage(getFrameIndex());
		swapchainImage = outSwapchainImage;
		swapchainImageView = outSwapchainImageView;
		swapchainImageIndex = outSwapchainImageIndex;

		// Reset the current fences, done here so the swapchain acquire doesn't stall the engine
		mEngineTextures->getSwapchain().reset(getFrameIndex());

		// Clear the draw image
		CVulkanUtils::transitionImage(cmd, mEngineTextures->mDrawImage->mImage, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

		constexpr VkClearColorValue color = { .float32 = {0.0, 0.0, 0.0} };
		constexpr VkImageSubresourceRange imageSubresourceRange { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
		vkCmdClearColorImage(cmd, mEngineTextures->mDrawImage->mImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &color, 1, &imageSubresourceRange);

		// Make the swapchain image into writeable mode before rendering
		CVulkanUtils::transitionImage(cmd, mEngineTextures->mDrawImage->mImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL);

		// Flush temporary frame data before it is used
		getCurrentFrame().mFrameResourceManager.flush();
	}

	{
		ZoneScopedN("Render Frame");
		{
			ZoneScopedN("Child Render");
			render(cmd);
		}

		CVulkanUtils::transitionImage(cmd, mEngineTextures->mDrawImage->mImage, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
		CVulkanUtils::transitionImage(cmd, mEngineTextures->mDepthImage->mImage, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);

		//begin a render pass  connected to our draw image
		VkRenderingAttachmentInfo colorAttachment = CVulkanUtils::createAttachmentInfo(mEngineTextures->mDrawImage->mImageView, nullptr, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
		VkRenderingAttachmentInfo depthAttachment = CVulkanUtils::createDepthAttachmentInfo(mEngineTextures->mDepthImage->mImageView, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);

		VkExtent2D extent {
			CEngine::get().getViewport().mExtent.x,
			CEngine::get().getViewport().mExtent.y
		};

		{
			ZoneScopedN("Update Scene Data");

			// Update Scene Data Buffer
			{

				mSceneData.mScreenSize = Vector2f((float)extent.width, (float)extent.height);
				mSceneData.mInvScreenSize = Vector2f(1.f / (float)extent.width, 1.f / (float)extent.height);

				mSceneData.mViewProj = CEngine::get().mMainCamera->getViewProjectionMatrix();

				//some default lighting parameters
				mSceneData.mAmbientColor = glm::vec4(.1f);
				mSceneData.mSunlightColor = glm::vec4(1.f);
				mSceneData.mSunlightDirection = glm::vec4(0,1,0.5,1.f);

				//allocate a new uniform buffer for the scene data
				//m_GPUSceneDataBuffer = renderer.getCurrentFrame().mFrameResourceManager.allocateBuffer(sizeof(Data), VMA_MEMORY_USAGE_CPU_TO_GPU, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
			}

			//write the buffer
			auto* sceneUniformData = static_cast<SceneData*>(mSceneDataBuffer->GetMappedData());
			*sceneUniformData = mSceneData;

			CVulkanResourceManager::updateGlobalBuffer(mSceneDataBuffer);
		}

		{
			ZoneScopedN("Render");

			VkRenderingInfo renderInfo = CVulkanUtils::createRenderingInfo(extent, &colorAttachment, &depthAttachment);
			vkCmdBeginRendering(cmd, &renderInfo);

			mBasePass->render(cmd);

			mSpritePass->render(cmd);

			vkCmdEndRendering(cmd);
		}
	}

	{
		ZoneScopedN("End Frame");

		CVulkanUtils::transitionImage(cmd, mEngineTextures->mDrawImage->mImage,VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

		// Make the swapchain image into presentable mode
		CVulkanUtils::transitionImage(cmd, swapchainImage,VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

		// Execute a copy from the draw image into the swapchain
		auto [width, height, depth] = mEngineTextures->mDrawImage->mImageExtent;
		CVulkanUtils::copyImageToImage(cmd, mEngineTextures->mDrawImage->mImage, swapchainImage, {width, height}, mEngineTextures->getSwapchain().mSwapchain->mSwapchain.extent);

		// Set swapchain layout so it can be used by ImGui
		CVulkanUtils::transitionImage(cmd, swapchainImage,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

		// Render other UI
		EngineUI::runEngineUI();

		// Add the UI to the swapchain
		EngineUI::render(cmd, mEngineTextures->getSwapchain().mSwapchain->mSwapchain.extent, swapchainImageView);

		// Set swapchain image layout to Present so we can show it on the screen
		CVulkanUtils::transitionImage(cmd, swapchainImage,VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

		//finalize the command buffer (we can no longer add commands, but it can now be executed)
		VK_CHECK(vkEndCommandBuffer(cmd));

		mEngineTextures->getSwapchain().submit(cmd, CVulkanDevice::getQueue(EQueueType::GRAPHICS).mQueue, getFrameIndex(), swapchainImageIndex);

		//increase the number of frames drawn
		mFrameNumber++;

		// Tell tracy we just rendered a frame
		FrameMark;
	}
}

bool CVulkanRenderer::waitForGpu() const {
	// Make sure the gpu is not working
	vkDeviceWaitIdle(CEngine::device());

	return mEngineTextures->getSwapchain().wait(getFrameIndex());
}

void CNullRenderer::render(VkCommandBuffer cmd) {
	//CVulkanUtils::transitionImage(cmd, mEngineTextures->mDrawImage->mImage,VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
}

#include "VulkanRenderer.h"

#include <cmath>
#include <iostream>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/glm.hpp>
#include <tracy/Tracy.hpp>
#include <tracy/TracyVulkan.hpp>

#include "DescriptorManager.h"
#include "VulkanDevice.h"
#include "VulkanUtils.h"
#include "vulkan/vk_enum_string_helper.h"
#include "VkBootstrap.h"

#include "Engine.h"
#include "ShaderCompiler.h"
#include "EngineSettings.h"
#include "EngineTextures.h"
#include "EngineBuffers.h"
#include "GpuScene.h"
#include "MeshLoader.h"
#include "PipelineBuilder.h"
#include "ResourceManager.h"

#define COMMAND_CATEGORY "Engine"
ADD_COMMAND(bool, UseVsync, true);
#undef COMMAND_CATEGORY

CVulkanRenderer::CVulkanRenderer(): mVSync(UseVsync.get()) {}

void CVulkanRenderer::immediateSubmit(std::function<void(VkCommandBuffer cmd)>&& function) {
	VkDevice device = CEngine::device();

	const CVulkanRenderer& renderer = CEngine::renderer();

	VkCommandBuffer cmd = renderer.mUploadContext._commandBuffer;

	//begin the command buffer recording. We will use this command buffer exactly once before resetting, so we tell vulkan that
	VkCommandBufferBeginInfo cmdBeginInfo = CVulkanInfo::createCommandBufferBeginInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

	VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));

	//execute the function
	function(cmd);

	VK_CHECK(vkEndCommandBuffer(cmd));

	VkSubmitInfo submit = CVulkanInfo::submitInfo(&cmd);

	VK_CHECK(vkResetFences(device, 1, &renderer.mUploadContext._uploadFence));

	//submit command buffer to the queue and execute it.
	// _uploadFence will now block until the graphic commands finish execution
	VK_CHECK(vkQueueSubmit(CEngine::renderer().mGraphicsQueue, 1, &submit, renderer.mUploadContext._uploadFence));

	VK_CHECK(vkWaitForFences(device, 1, &renderer.mUploadContext._uploadFence, true, 1000000000));

	// reset the command buffers inside the command pool
	VK_CHECK(vkResetCommandPool(device, renderer.mUploadContext._commandPool, 0));
}

void CVulkanRenderer::init() {
	// Ensure the renderer is only created once
	astsOnce(CVulkanRenderer);

	// Use vkb to get a Graphics queue
	mGraphicsQueue = CEngine::device().get_queue(vkb::QueueType::graphics).value();
	mGraphicsQueueFamily = CEngine::device().get_queue_index(vkb::QueueType::graphics).value();

	VkCommandPoolCreateInfo uploadCommandPoolInfo = CVulkanInfo::createCommandPoolInfo(CEngine::renderer().mGraphicsQueueFamily);
	//create pool for upload context
	mUploadContext._commandPool = mGlobalResourceManager.allocateCommandPool(uploadCommandPoolInfo);

	//allocate the default command buffer that we will use for the instant commands
	VkCommandBufferAllocateInfo cmdAllocInfo = CVulkanInfo::createCommandAllocateInfo(mUploadContext._commandPool, 1);
	mUploadContext._commandBuffer = CResourceManager::allocateCommandBuffer(cmdAllocInfo);

	VkFenceCreateInfo fenceCreateInfo = CVulkanInfo::createFenceInfo(VK_FENCE_CREATE_SIGNALED_BIT);
	mUploadContext._uploadFence = mGlobalResourceManager.allocateFence(fenceCreateInfo);

	{
		// Create a command pool for commands submitted to the graphics queue.
		// We also want the pool to allow for resetting of individual command buffers
		const VkCommandPoolCreateInfo commandPoolInfo = CVulkanInfo::createCommandPoolInfo(
			mGraphicsQueueFamily, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

		for (auto &frame: mFrames) {
			frame.mCommandPool = mGlobalResourceManager.allocateCommandPool(commandPoolInfo);

			// Allocate the default command buffer that we will use for rendering
			VkCommandBufferAllocateInfo cmdAllocInfo = CVulkanInfo::createCommandAllocateInfo(frame.mCommandPool, 1);

			frame.mMainCommandBuffer = CResourceManager::allocateCommandBuffer(cmdAllocInfo);

			std::vector<SDescriptorAllocator::PoolSizeRatio> frameSizes = {
				{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 3 },
				{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 3 },
				{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 3 },
				{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4 },
			};

			// TODO: DescriptorAllocator as part of ResourceManager
			frame.mDescriptorAllocator = SDescriptorAllocator();
			frame.mDescriptorAllocator.init(1000, frameSizes);

			mGlobalResourceManager.pushF([&] {
				frame.mDescriptorAllocator.destroy();
			});

			frame.mTracyContext = TracyVkContext(CEngine::physicalDevice(), CEngine::device(), mGraphicsQueue, frame.mMainCommandBuffer);
		}
	}

	//create a descriptor pool that will hold 10 sets with 1 image each
	std::vector<SDescriptorAllocator::PoolSizeRatio> sizes =
	{
		{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1},
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1}
	};

	mGlobalDescriptorAllocator.init(10, sizes);

	mMeshLoader = mGlobalResourceManager.createDestroyable<CMeshLoader>();

	mEngineTextures = mGlobalResourceManager.createDestroyable<CEngineTextures>();

	mEngineBuffers = mGlobalResourceManager.createDestroyable<CEngineBuffers>(this);

	mGPUScene = mGlobalResourceManager.createDestroyable<CGPUScene>(this);

	// Setup Dear ImGui
	CEngineSettings::init(this, mGraphicsQueue, mEngineTextures->getSwapchain().mFormat);
}

void CVulkanRenderer::destroy() {

	//mGPUScene->m_LoadedScenes.clear();
	mMeshLoader->mLoadedModels.clear();

	CEngineSettings::destroy();

	for (auto& frame : mFrames) {
		TracyVkDestroy(frame.mTracyContext);

		frame.mDescriptorAllocator.clear();
		frame.mFrameResourceManager.flush();
	}

	mGlobalDescriptorAllocator.destroy();

	mGlobalResourceManager.flush();
}

void CVulkanRenderer::draw() {

	ZoneScopedN("render");

	if (mVSync != UseVsync.get()) {
		mVSync = UseVsync.get();
		msgs("Reallocating Swapchain to {}", UseVsync.get() ? "enable VSync." : "disable VSync.");
		mEngineTextures->getSwapchain().setDirty();
	}

	// Make sure that the swapchain is not dirty before recreating it
	while (mEngineTextures->getSwapchain().isDirty()) {
		// Wait for gpu before recreating swapchain
		waitForGpu();

		mEngineTextures->reallocate(UseVsync.get());
	}

	CEngineSettings::begin();

	{
		// Wait for the previous render to stop
		mEngineTextures->getSwapchain().wait(getFrameIndex());
	}

	// Get command buffer from current frame
	VkCommandBuffer cmd = getCurrentFrame().mMainCommandBuffer;

	VkImage swapchainImage;
	VkImageView swapchainImageView;
	uint32 swapchainImageIndex;

	{
		ZoneScopedN("Initialize Frame");

		VK_CHECK(vkResetCommandBuffer(cmd, 0));

		// Tell Vulkan the buffer will only be used once
		VkCommandBufferBeginInfo cmdBeginInfo = CVulkanInfo::createCommandBufferBeginInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
		VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));

		// Get the current swapchain image
		const auto [outSwapchainImage, outSwapchainImageView, outSwapchainImageIndex] = mEngineTextures->getSwapchain().getSwapchainImage(getFrameIndex());
		swapchainImage = outSwapchainImage;
		swapchainImageView = outSwapchainImageView;
		swapchainImageIndex = outSwapchainImageIndex;

		// Reset the current fences, done here so the swapchain acquire doesn't stall the engine
		mEngineTextures->getSwapchain().reset(getFrameIndex());
	}

	// Make the swapchain image into writeable mode before rendering
	CVulkanUtils::transitionImage(cmd, mEngineTextures->mDrawImage->mImage, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);

	{
		ZoneScopedN("Flush Frame Data");
		// Flush temporary frame data before it is used
		getCurrentFrame().mDescriptorAllocator.clear();
		getCurrentFrame().mFrameResourceManager.flush();
	}

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
		mEngineTextures->mDrawImage->mImageExtent.width,
		mEngineTextures->mDrawImage->mImageExtent.height
	};

	{
		ZoneScopedN("Run GPU Scene");

		VkRenderingInfo renderInfo = CVulkanUtils::createRenderingInfo(extent, &colorAttachment, &depthAttachment);
		vkCmdBeginRendering(cmd, &renderInfo);

		mGPUScene->render(this, cmd);

		vkCmdEndRendering(cmd);
	}

	CVulkanUtils::transitionImage(cmd, mEngineTextures->mDrawImage->mImage,VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

	/*CVulkanUtils::transitionImage(cmd, mEngineTextures->mDrawImage->mImage,VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
	CVulkanUtils::transitionImage(cmd, mEngineTextures->mDepthImage->mImage,VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

	auto [width, height, depth] = mEngineTextures->mDrawImage->mImageExtent;
	CVulkanUtils::copyImageToImage(cmd, mEngineTextures->mDepthImage->mImage, mEngineTextures->mDrawImage->mImage, {width, height}, {mEngineTextures->mDepthImage->mImageExtent.width, mEngineTextures->mDepthImage->mImageExtent.height});
	CVulkanUtils::transitionImage(cmd, mEngineTextures->mDrawImage->mImage,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
*/
	// Make the swapchain image into presentable mode
	CVulkanUtils::transitionImage(cmd, swapchainImage,VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

	// Execute a copy from the draw image into the swapchain
	auto [width, height, depth] = mEngineTextures->mDrawImage->mImageExtent;
	CVulkanUtils::copyImageToImage(cmd, mEngineTextures->mDrawImage->mImage, swapchainImage, {width, height}, mEngineTextures->getSwapchain().mSwapchain->extent);

	// Set swapchain layout so it can be used by ImGui
	CVulkanUtils::transitionImage(cmd, swapchainImage,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

	// Render Engine settings
	CEngineSettings::render(cmd, mEngineTextures->getSwapchain().mSwapchain->extent, swapchainImageView);

	// Set swapchain image layout to Present so we can show it on the screen
	CVulkanUtils::transitionImage(cmd, swapchainImage,VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

	//finalize the command buffer (we can no longer add commands, but it can now be executed)
	VK_CHECK(vkEndCommandBuffer(cmd));

	mEngineTextures->getSwapchain().submit(cmd, mGraphicsQueue, getFrameIndex(), swapchainImageIndex);

	//increase the number of frames drawn
	mFrameNumber++;

	// Tell tracy we just rendered a frame
	FrameMark;
}

void CVulkanRenderer::waitForGpu() const {
	// Make sure the gpu is not working
	vkDeviceWaitIdle(CEngine::device());

	mEngineTextures->getSwapchain().wait(getFrameIndex());
}

void CNullRenderer::render(VkCommandBuffer cmd) {
	//CVulkanUtils::transitionImage(cmd, mEngineTextures->mDrawImage->mImage,VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
}

#include "include/VulkanRenderer.h"

#include <cmath>
#include <iostream>

#include "VulkanDevice.h"
#include "VulkanUtils.h"
#include "vulkan/vk_enum_string_helper.h"

#define VMA_IMPLEMENTATION
#include "vma/vk_mem_alloc.h"

CVulkanRenderer::CVulkanRenderer() {
	// Ensure the renderer is only created once
	assertOnce(CVulkanRenderer);

	// Use vkb to get a Graphics queue
	m_GraphicsQueue = CEngine::get().getDevice().getDevice().get_queue(vkb::QueueType::graphics).value();
	m_GraphicsQueueFamily = CEngine::get().getDevice().getDevice().get_queue_index(vkb::QueueType::graphics).value();

	{
		// Create a command pool for commands submitted to the graphics queue.
		// We also want the pool to allow for resetting of individual command buffers
		const VkCommandPoolCreateInfo commandPoolInfo = CVulkanInfo::CreateCommandPoolInfo(
			m_GraphicsQueueFamily, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

		for (auto &frame: m_Frames) {
			VK_CHECK(vkCreateCommandPool(CEngine::get().getDevice().getDevice(), &commandPoolInfo, nullptr, &frame.mCommandPool));

			// Allocate the default command buffer that we will use for rendering
			VkCommandBufferAllocateInfo cmdAllocInfo = CVulkanInfo::CreateCommandAllocateInfo(frame.mCommandPool, 1);

			VK_CHECK(vkAllocateCommandBuffers(CEngine::get().getDevice().getDevice(), &cmdAllocInfo, &frame.mMainCommandBuffer));
		}
	}

	// initialize the memory allocator
	{
		VmaAllocatorCreateInfo allocatorInfo = {};
		allocatorInfo.physicalDevice = CEngine::get().getDevice().getPhysicalDevice();
		allocatorInfo.device = CEngine::get().getDevice().getDevice();
		allocatorInfo.instance = CEngine::get().getDevice().getInstance();
		allocatorInfo.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
		vmaCreateAllocator(&allocatorInfo, &m_Allocator);

		m_DeletionQueue.push([&] {
			vmaDestroyAllocator(m_Allocator);
		});
	}

	{
		//hardcoding the draw format to 32 bit float
		m_DrawImage.mImageFormat = VK_FORMAT_R16G16B16A16_SFLOAT;

		VkImageUsageFlags drawImageUsages{};
		drawImageUsages |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
		drawImageUsages |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
		drawImageUsages |= VK_IMAGE_USAGE_STORAGE_BIT;
		drawImageUsages |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

		VkImageCreateInfo imageInfo = CVulkanInfo::CreateImageInfo(m_DrawImage.mImageFormat, drawImageUsages, VkExtent3D(CEngine::get().getWindow().mExtent.x, CEngine::get().getWindow().mExtent.y, 1));

		//for the draw image, we want to allocate it from gpu local memory
		VmaAllocationCreateInfo imageAllocationInfo = {};
		imageAllocationInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
		imageAllocationInfo.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		//allocate and create the image
		vmaCreateImage(m_Allocator, &imageInfo, &imageAllocationInfo, &m_DrawImage.mImage, &m_DrawImage.mAllocation, nullptr);

		//build a image-view for the draw image to use for rendering
		VkImageViewCreateInfo imageViewInfo = CVulkanInfo::CreateImageViewInfo(m_DrawImage.mImageFormat, m_DrawImage.mImage, VK_IMAGE_ASPECT_COLOR_BIT);

		VK_CHECK(vkCreateImageView(CEngine::get().getDevice().getDevice(), &imageViewInfo, nullptr, &m_DrawImage.mImageView));

		//add to deletion queues
		m_ImageDeletionQueue.push([this] {
			vkDestroyImageView(CEngine::get().getDevice().getDevice(), m_DrawImage.mImageView, nullptr);
			vmaDestroyImage(m_Allocator, m_DrawImage.mImage, m_DrawImage.mAllocation);
		});
	}

	m_Swapchain = std::make_unique<CSwapchain>();
}

void CVulkanRenderer::destroy() {
	for (auto & mFrame : m_Frames) {
		vkDestroyCommandPool(CEngine::get().getDevice().getDevice(), mFrame.mCommandPool, nullptr);
		mFrame.mDeletionQueue.flush();
	}

	m_ImageDeletionQueue.flush();
	m_DeletionQueue.flush();

	m_Swapchain->cleanup();
}

void CVulkanRenderer::render() {
	// Make sure that the swapchain is not dirty before recreating it
	while (m_Swapchain->isDirty()) {
		// Wait for gpu before recreating swapchain
		waitForGpu();

		m_Swapchain->recreate();
	}

	// Wait for the previous render to stop
	m_Swapchain->wait(getFrameIndex());

	// Get command buffer from current frame
	VkCommandBuffer cmd = getCurrentFrame().mMainCommandBuffer;
	VK_CHECK(vkResetCommandBuffer(cmd, 0));

	// Tell Vulkan the buffer will only be used once
	VkCommandBufferBeginInfo cmdBeginInfo = CVulkanInfo::CreateCommandBufferBeginInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
	VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));

	// Get the current swapchain image
	const auto [swapchainImage, swapchainImageIndex] = m_Swapchain->getSwapchainImage(getFrameIndex());

	// Reset the current fences, done here so the swapchain acquire doesn't stall the engine
	m_Swapchain->reset(getFrameIndex());

	// Flush temporary frame data
	m_Frames[getFrameIndex()].mDeletionQueue.flush();

	// Make the swapchain image into writeable mode before rendering
	CVulkanUtils::TransitionImage(cmd, m_DrawImage.mImage, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);

	drawBackground(cmd);

	// Make the swapchain image into presentable mode
	CVulkanUtils::TransitionImage(cmd, m_DrawImage.mImage,VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
	CVulkanUtils::TransitionImage(cmd, swapchainImage,VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

	// Execute a copy from the draw image into the swapchain
	CVulkanUtils::CopyImageToImage(cmd, m_DrawImage.mImage, swapchainImage, VkExtent2D(CEngine::get().getWindow().mExtent.x, CEngine::get().getWindow().mExtent.y), m_Swapchain->mSwapchain.extent);

	// Set swapchain image layout to Present so we can show it on the screen
	CVulkanUtils::TransitionImage(cmd, swapchainImage,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

	//finalize the command buffer (we can no longer add commands, but it can now be executed)
	VK_CHECK(vkEndCommandBuffer(cmd));

	m_Swapchain->submit(cmd, m_GraphicsQueue, getFrameIndex(), swapchainImageIndex);

	//increase the number of frames drawn
	m_FrameNumber++;
}

void CVulkanRenderer::drawBackground(VkCommandBuffer cmd) const {
	//make a clear-color from game time.
	VkClearColorValue clearValue;
	float flash = std::abs(std::sin(CEngine::get().getTime()));
	clearValue = { { 0.0f, 0.0f, flash, 1.0f } };

	VkImageSubresourceRange clearRange = CVulkanUtils::ImageSubresourceRange(VK_IMAGE_ASPECT_COLOR_BIT);

	//clear image
	vkCmdClearColorImage(cmd, m_DrawImage.mImage, VK_IMAGE_LAYOUT_GENERAL, &clearValue, 1, &clearRange);
}

void CVulkanRenderer::waitForGpu() {
	// Make sure the gpu is not working
	vkDeviceWaitIdle(CEngine::get().getDevice().getDevice());

	m_Swapchain->wait(getFrameIndex());
}

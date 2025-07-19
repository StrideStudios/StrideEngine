#include "include/VulkanRenderer.h"

#include <cmath>
#include <iostream>

#include "VulkanDevice.h"
#include "VulkanUtils.h"
#include "GLFW/glfw3.h"

#define VMA_IMPLEMENTATION
#include "vma/vk_mem_alloc.h"

constexpr static double TARGET_FRAMERATE = 180.0;

CVulkanRenderer::CVulkanRenderer(const CVulkanDevice& inVulkanDevice): m_VulkanDevice(inVulkanDevice) {
	// Use vkb to get a Graphics queue
	m_GraphicsQueue = m_VulkanDevice.getDevice().get_queue(vkb::QueueType::graphics).value();
	m_GraphicsQueueFamily = m_VulkanDevice.getDevice().get_queue_index(vkb::QueueType::graphics).value();

	{
		// Create a command pool for commands submitted to the graphics queue.
		// We also want the pool to allow for resetting of individual command buffers
		const VkCommandPoolCreateInfo commandPoolInfo = CVulkanInfo::CreateCommandPoolInfo(
			m_GraphicsQueueFamily, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

		for (auto &frame: m_Frames) {
			VK_CHECK(vkCreateCommandPool(m_VulkanDevice.getDevice(), &commandPoolInfo, nullptr, &frame.mCommandPool));

			// Allocate the default command buffer that we will use for rendering
			VkCommandBufferAllocateInfo cmdAllocInfo = CVulkanInfo::CreateCommandAllocateInfo(frame.mCommandPool, 1);

			VK_CHECK(vkAllocateCommandBuffers(m_VulkanDevice.getDevice(), &cmdAllocInfo, &frame.mMainCommandBuffer));
		}
	}

	{
		//create syncronization structures
		//one fence to control when the gpu has finished rendering the frame,
		//and 2 semaphores to syncronize rendering with swapchain
		//we want the fence to start signalled so we can wait on it on the first frame
		VkFenceCreateInfo fenceCreateInfo = CVulkanInfo::CreateFenceInfo(VK_FENCE_CREATE_SIGNALED_BIT);
		VkSemaphoreCreateInfo semaphoreCreateInfo = CVulkanInfo::CreateSemaphoreInfo();

		for (auto &mFrame: m_Frames) {
			VK_CHECK(vkCreateFence(m_VulkanDevice.getDevice(), &fenceCreateInfo, nullptr, &mFrame.mRenderFence));
			VK_CHECK(vkCreateFence(m_VulkanDevice.getDevice(), &fenceCreateInfo, nullptr, &mFrame.mPresentFence));

			VK_CHECK(
				vkCreateSemaphore(m_VulkanDevice.getDevice(), &semaphoreCreateInfo, nullptr, &mFrame.mSwapchainSemaphore));
			VK_CHECK(vkCreateSemaphore(m_VulkanDevice.getDevice(), &semaphoreCreateInfo, nullptr, &mFrame.mRenderSemaphore))
			;
		}
	}

	{
		vkb::SwapchainBuilder swapchainBuilder{ m_VulkanDevice.getPhysicalDevice(),m_VulkanDevice.getDevice(),m_VulkanDevice.getSurface() };

		m_VkFormat = VK_FORMAT_B8G8R8A8_UNORM;

		vkb::Swapchain vkbSwapchain = swapchainBuilder
			//.use_default_format_selection()
			.set_desired_format(VkSurfaceFormatKHR{ .format = m_VkFormat, .colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR })
			//use vsync present mode
			.set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)
			.set_desired_extent(1920, 1080) //TODO: global width/height (or input idk)
			.add_image_usage_flags(VK_IMAGE_USAGE_TRANSFER_DST_BIT)
			.build()
			.value();

		m_VkSwapchainExtent = vkbSwapchain.extent;
		//store swapchain and its related images
		m_VkSwapchain = vkbSwapchain.swapchain;
		m_VkSwapchainImages = vkbSwapchain.get_images().value();
		m_VkSwapchainImageViews = vkbSwapchain.get_image_views().value();

		// Allocate render semaphores
		/*m_RenderSemaphores.resize(m_VkSwapchainImages.size());
		for (int32 i = 0; i < m_VkSwapchainImages.size(); ++i) {
			VK_CHECK(vkCreateSemaphore(vulkanDevice.getDevice(), &semaphoreCreateInfo, nullptr, &m_RenderSemaphores[i]));
		}*/
	}

	// initialize the memory allocator
	{
		VmaAllocatorCreateInfo allocatorInfo = {};
		allocatorInfo.physicalDevice = m_VulkanDevice.getPhysicalDevice();
		allocatorInfo.device = m_VulkanDevice.getDevice();
		allocatorInfo.instance = m_VulkanDevice.getInstance();
		allocatorInfo.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
		vmaCreateAllocator(&allocatorInfo, &m_Allocator);

		m_DeletionQueue.push([&] {
			vmaDestroyAllocator(m_Allocator);
		});
	}

	{
		//draw image size will match the window
		VkExtent3D drawImageExtent = {
			1920,
			1080, //TODO: dynamic window
			1
		};

		//hardcoding the draw format to 32 bit float
		m_DrawImage.mImageFormat = VK_FORMAT_R16G16B16A16_SFLOAT;
		m_DrawImage.mImageExtent = drawImageExtent;

		VkImageUsageFlags drawImageUsages{};
		drawImageUsages |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
		drawImageUsages |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
		drawImageUsages |= VK_IMAGE_USAGE_STORAGE_BIT;
		drawImageUsages |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

		VkImageCreateInfo rimg_info = CVulkanInfo::CreateImageInfo(m_DrawImage.mImageFormat, drawImageUsages, drawImageExtent);

		//for the draw image, we want to allocate it from gpu local memory
		VmaAllocationCreateInfo rimg_allocinfo = {};
		rimg_allocinfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
		rimg_allocinfo.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		//allocate and create the image
		vmaCreateImage(m_Allocator, &rimg_info, &rimg_allocinfo, &m_DrawImage.mImage, &m_DrawImage.mAllocation, nullptr);

		//build a image-view for the draw image to use for rendering
		VkImageViewCreateInfo rview_info = CVulkanInfo::CreateImageViewInfo(m_DrawImage.mImageFormat, m_DrawImage.mImage, VK_IMAGE_ASPECT_COLOR_BIT);

		VK_CHECK(vkCreateImageView(m_VulkanDevice.getDevice(), &rview_info, nullptr, &m_DrawImage.mImageView));

		//add to deletion queues
		m_DeletionQueue.push([=] {
			vkDestroyImageView(m_VulkanDevice.getDevice(), m_DrawImage.mImageView, nullptr);
			vmaDestroyImage(m_Allocator, m_DrawImage.mImage, m_DrawImage.mAllocation);
		});
	}
}

void CVulkanRenderer::draw() {

	m_DeltaTime = glfwGetTime() - m_PreviousTime;

	m_FrameRate = (int32)(1.0 / m_DeltaTime);

	// Frame cap
	// This ensures the program has waited long enough between draws so it doesn't overdraw
	if (m_FrameRate > TARGET_FRAMERATE)
		return;

	m_PreviousTime = glfwGetTime();

	// Easy way to visualize FPS before text is implemented
	std::string title = "Stride Engine: ";
	title.append(std::to_string(m_FrameRate));
	glfwSetWindowTitle(m_VulkanDevice.getWindow(), title.c_str());

	// Wait until the gpu has finished rendering the last frame. Timeout of 1 Second
	VK_CHECK(vkWaitForFences(m_VulkanDevice.getDevice(), 1, &getCurrentFrame().mRenderFence, true, 1000000000));
	VK_CHECK(vkResetFences(m_VulkanDevice.getDevice(), 1, &getCurrentFrame().mRenderFence));

	// Wait until the current present is done
	VK_CHECK(vkWaitForFences(m_VulkanDevice.getDevice(), 1, &getCurrentFrame().mPresentFence, true, 1000000000));
	VK_CHECK(vkResetFences(m_VulkanDevice.getDevice(), 1, &getCurrentFrame().mPresentFence));

	// Flush temporary frame data
	getCurrentFrame().mDeletionQueue.flush();

	uint32 swapchainImageIndex;
	VK_CHECK(vkAcquireNextImageKHR(m_VulkanDevice.getDevice(), m_VkSwapchain, 1000000000, getCurrentFrame().mSwapchainSemaphore, nullptr, &swapchainImageIndex));

	// Naming it cmd for shorter writing
	VkCommandBuffer cmd = getCurrentFrame().mMainCommandBuffer;

	// Now that we are sure that the commands finished executing, we can safely
	// Reset the command buffer to begin recording again.
	VK_CHECK(vkResetCommandBuffer(cmd, 0));

	// Begin the command buffer recording. We will use this command buffer exactly once, so we want to let vulkan know that
	VkCommandBufferBeginInfo cmdBeginInfo = CVulkanInfo::CreateCommandBufferBeginInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

	// Start the command buffer recording
	VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));

	// Make the swapchain image into writeable mode before rendering
	CVulkanUtils::TransitionImage(cmd, m_DrawImage.mImage, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);

	drawBackground(cmd);

	// Make the swapchain image into presentable mode
	CVulkanUtils::TransitionImage(cmd, m_DrawImage.mImage,VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
	CVulkanUtils::TransitionImage(cmd, m_VkSwapchainImages[swapchainImageIndex],VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

	// Execute a copy from the draw image into the swapchain
	CVulkanUtils::CopyImageToImage(cmd, m_DrawImage.mImage, m_VkSwapchainImages[swapchainImageIndex], m_DrawExtent, m_VkSwapchainExtent);

	// Set swapchain image layout to Present so we can show it on the screen
	CVulkanUtils::TransitionImage(cmd, m_VkSwapchainImages[swapchainImageIndex],VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

	//finalize the command buffer (we can no longer add commands, but it can now be executed)
	VK_CHECK(vkEndCommandBuffer(cmd));

	//prepare the submission to the queue.
	//we want to wait on the _presentSemaphore, as that semaphore is signaled when the swapchain is ready
	//we will signal the _renderSemaphore, to signal that rendering has finished

	VkCommandBufferSubmitInfo cmdinfo = CVulkanInfo::SubmitCommandBufferInfo(cmd);

	VkSemaphoreSubmitInfo waitInfo = CVulkanInfo::SubmitSemaphoreInfo(VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR, getCurrentFrame().mSwapchainSemaphore);

	//VkSemaphore renderSemaphore = m_RenderSemaphores[swapchainImageIndex];
	VkSemaphoreSubmitInfo signalInfo = CVulkanInfo::SubmitSemaphoreInfo(VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT, getCurrentFrame().mRenderSemaphore);

	VkSubmitInfo2 submit = CVulkanInfo::SubmitInfo(&cmdinfo,&signalInfo,&waitInfo);

	//submit command buffer to the queue and execute it.
	// _renderFence will now block until the graphic commands finish execution
	VK_CHECK(vkQueueSubmit2(m_GraphicsQueue, 1, &submit, getCurrentFrame().mRenderFence));

	VkSwapchainPresentFenceInfoEXT presentFenceInfo = {};
	presentFenceInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_PRESENT_FENCE_INFO_EXT;
	presentFenceInfo.pFences = &getCurrentFrame().mPresentFence;
	presentFenceInfo.swapchainCount = 1;

	//prepare present
	// this will put the image we just rendered to into the visible window.
	// we want to wait on the _renderSemaphore for that,
	// as its necessary that drawing commands have finished before the image is displayed to the user
	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.pNext = &presentFenceInfo;
	presentInfo.pSwapchains = &m_VkSwapchain;
	presentInfo.swapchainCount = 1;

	presentInfo.pWaitSemaphores = &getCurrentFrame().mRenderSemaphore;
	presentInfo.waitSemaphoreCount = 1;

	presentInfo.pImageIndices = &swapchainImageIndex;

	VK_CHECK(vkQueuePresentKHR(m_GraphicsQueue, &presentInfo));

	//increase the number of frames drawn
	m_FrameNumber++;
}

void CVulkanRenderer::drawBackground(VkCommandBuffer cmd) const {
	//make a clear-color from frame number. This will flash with a 120 frame period.
	VkClearColorValue clearValue;
	float flash = std::abs(std::sin(m_FrameNumber / 120.f));
	clearValue = { { 0.0f, 0.0f, flash, 1.0f } };

	VkImageSubresourceRange clearRange = CVulkanUtils::ImageSubresourceRange(VK_IMAGE_ASPECT_COLOR_BIT);

	//clear image
	vkCmdClearColorImage(cmd, m_DrawImage.mImage, VK_IMAGE_LAYOUT_GENERAL, &clearValue, 1, &clearRange);
}

void CVulkanRenderer::cleanup() {
	//make sure the gpu has stopped doing its things
	vkDeviceWaitIdle(m_VulkanDevice.getDevice());

	// Wait until the current present is done so swapchain resources can be safely destroyed
	VK_CHECK(vkWaitForFences(m_VulkanDevice.getDevice(), 1, &getCurrentFrame().mPresentFence, true, 1000000000));
	VK_CHECK(vkResetFences(m_VulkanDevice.getDevice(), 1, &getCurrentFrame().mPresentFence));

	for (auto & mFrame : m_Frames) {

		//already written from before
		vkDestroyCommandPool(m_VulkanDevice.getDevice(), mFrame.mCommandPool, nullptr);

		//destroy sync objects
		vkDestroyFence(m_VulkanDevice.getDevice(), mFrame.mRenderFence, nullptr);
		vkDestroyFence(m_VulkanDevice.getDevice(), mFrame.mPresentFence, nullptr);
		vkDestroySemaphore(m_VulkanDevice.getDevice() ,mFrame.mSwapchainSemaphore, nullptr);
		vkDestroySemaphore(m_VulkanDevice.getDevice() ,mFrame.mRenderSemaphore, nullptr);

		mFrame.mDeletionQueue.flush();
	}

	m_DeletionQueue.flush();

	vkDestroySwapchainKHR(m_VulkanDevice.getDevice(), m_VkSwapchain, nullptr);

	// destroy swapchain resources and optional render semaphores
	for (int32 i = 0; i < m_VkSwapchainImageViews.size(); ++i) {
		//vkDestroySemaphore(vulkanDevice.getDevice(), m_RenderSemaphores[i], nullptr);
		vkDestroyImageView(m_VulkanDevice.getDevice(), m_VkSwapchainImageViews[i], nullptr);
	}
}

#include "Swapchain.h"

#include "Engine.h"
#include "VulkanDevice.h"
#include "VulkanUtils.h"

#include "vulkan/vk_enum_string_helper.h"

CSwapchain::CSwapchain() {

	init(VK_NULL_HANDLE, true);

	// Allocate render semaphores
	/*m_RenderSemaphores.resize(m_VkSwapchainImages.size());
	for (int32 i = 0; i < m_VkSwapchainImages.size(); ++i) {
		VK_CHECK(vkCreateSemaphore(vulkanDevice.getDevice(), &semaphoreCreateInfo, nullptr, &m_RenderSemaphores[i]));
	}*/

	{
		// Create one fence to control when the gpu has finished rendering the frame,
		// And 2 semaphores to synchronize rendering with swapchain
		VkFenceCreateInfo fenceCreateInfo = CVulkanInfo::createFenceInfo(VK_FENCE_CREATE_SIGNALED_BIT);
		VkSemaphoreCreateInfo semaphoreCreateInfo = CVulkanInfo::createSemaphoreInfo();

		for (auto &mFrame: m_Frames) {

			mFrame.mRenderFence = m_ResourceManager.allocateFence(fenceCreateInfo);
			mFrame.mPresentFence = m_ResourceManager.allocateFence(fenceCreateInfo);

			mFrame.mSwapchainSemaphore = m_ResourceManager.allocateSemaphore(semaphoreCreateInfo);
			mFrame.mRenderSemaphore = m_ResourceManager.allocateSemaphore(semaphoreCreateInfo);
		}

	}
}

CSwapchain::~CSwapchain() = default;

CSwapchain::operator VkSwapchainKHR() const  {
	return *mSwapchain;
}

void CSwapchain::init(const VkSwapchainKHR oldSwapchain, const bool inUseVSync) {
	mFormat = VK_FORMAT_R8G8B8A8_UNORM;

	const auto& vkbSwapchain = vkb::SwapchainBuilder{CEngine::device()}
		//.use_default_format_selection()
		.set_old_swapchain(oldSwapchain)
		.set_desired_format(VkSurfaceFormatKHR{ .format = mFormat, .colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR })
		//use vsync present mode
		.set_desired_present_mode(inUseVSync ? VK_PRESENT_MODE_FIFO_KHR : VK_PRESENT_MODE_IMMEDIATE_KHR)
		.set_desired_extent(CEngine::get().getWindow().mExtent.x, CEngine::get().getWindow().mExtent.y)
		.add_image_usage_flags(VK_IMAGE_USAGE_TRANSFER_DST_BIT)
		.build();

	if (!vkbSwapchain.has_value()) {
		errs("Swapchain failed to recreate after change. Vulkan Error: {}", string_VkResult(vkbSwapchain.full_error().vk_result));
	}

	// If the swapchain is dirty we know these have been allocated
	if (m_Dirty) {
		// destroy swapchain resources and optional render semaphores
		for (int32 i = 0; i < mSwapchainImageViews.size(); ++i) {
			//vkDestroySemaphore(vulkanDevice.getDevice(), m_RenderSemaphores[i], nullptr);
			vkDestroyImageView(CEngine::device(), mSwapchainImageViews[i], nullptr);
		}

		vkb::destroy_swapchain(*mSwapchain);
	}

	//store swapchain and its related images
	mSwapchain = std::make_unique<vkb::Swapchain>(vkbSwapchain.value());
	mSwapchainImages = mSwapchain->get_images().value();
	mSwapchainImageViews = mSwapchain->get_image_views().value();
}

void CSwapchain::recreate(bool inUseVSync) {

	init(*mSwapchain, inUseVSync);

	m_Dirty = false;
}

void CSwapchain::cleanup() {

	m_ResourceManager.flush();

	// destroy swapchain resources and optional render semaphores
	for (int32 i = 0; i < mSwapchainImageViews.size(); ++i) {
		//vkDestroySemaphore(vulkanDevice.getDevice(), m_RenderSemaphores[i], nullptr);
		vkDestroyImageView(CEngine::device(), mSwapchainImageViews[i], nullptr);
	}

	vkb::destroy_swapchain(*mSwapchain);
}

std::pair<VkImage, uint32> CSwapchain::getSwapchainImage(const uint32 inCurrentFrameIndex) {
	uint32 swapchainImageIndex = 0;
	SWAPCHAIN_CHECK(vkAcquireNextImageKHR(CEngine::device(), *mSwapchain, 1000000000, m_Frames[inCurrentFrameIndex].mSwapchainSemaphore, nullptr, &swapchainImageIndex), setDirty());

	return {mSwapchainImages[swapchainImageIndex], swapchainImageIndex};
}

void CSwapchain::wait(const uint32 inCurrentFrameIndex) const {
	auto& frame = m_Frames[inCurrentFrameIndex];
	VK_CHECK(vkWaitForFences(CEngine::device(), 1, &frame.mRenderFence, true, 1000000000));
	VK_CHECK(vkWaitForFences(CEngine::device(), 1, &frame.mPresentFence, true, 1000000000));
}

void CSwapchain::reset(const uint32 inCurrentFrameIndex) const {
	auto& frame = m_Frames[inCurrentFrameIndex];
	VK_CHECK(vkResetFences(CEngine::device(), 1, &frame.mRenderFence));
	VK_CHECK(vkResetFences(CEngine::device(), 1, &frame.mPresentFence));
}

void CSwapchain::submit(const VkCommandBuffer inCmd, const VkQueue inGraphicsQueue, uint32 inCurrentFrameIndex, const uint32 inSwapchainImageIndex) {
	const auto& frame = m_Frames[inCurrentFrameIndex];

	// Submit
	{
		VkCommandBufferSubmitInfo cmdInfo = CVulkanInfo::submitCommandBufferInfo(inCmd);

		VkSemaphoreSubmitInfo waitInfo = CVulkanInfo::submitSemaphoreInfo(VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR, frame.mSwapchainSemaphore);
		VkSemaphoreSubmitInfo signalInfo = CVulkanInfo::submitSemaphoreInfo(VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT, frame.mRenderSemaphore);

		VkSubmitInfo2 submit = CVulkanInfo::submitInfo(&cmdInfo,&signalInfo,&waitInfo);
		VK_CHECK(vkQueueSubmit2(inGraphicsQueue, 1, &submit, frame.mRenderFence));
	}

	// Present
	{
		VkSwapchainPresentFenceInfoEXT presentFenceInfo {
			.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_PRESENT_FENCE_INFO_EXT,
			.swapchainCount = 1,
			.pFences = &frame.mPresentFence
		};

		const auto presentInfo = VkPresentInfoKHR {
			.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
			.pNext = &presentFenceInfo,
			.waitSemaphoreCount = 1,
			.pWaitSemaphores = &frame.mRenderSemaphore,
			.swapchainCount = 1,
			.pSwapchains = &mSwapchain->swapchain,
			.pImageIndices = &inSwapchainImageIndex,
		};

		SWAPCHAIN_CHECK(vkQueuePresentKHR(inGraphicsQueue, &presentInfo), setDirty());
	}
}
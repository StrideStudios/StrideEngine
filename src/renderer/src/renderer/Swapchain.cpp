#include "renderer/Swapchain.h"
#include "Viewport.h"
#include "renderer/VulkanDevice.h"
#include "renderer/VulkanRenderer.h"
#include "renderer/VulkanResourceManager.h"
#include "VulkanUtils.h"
#include "tracy/Tracy.hpp"

#include "vulkan/vk_enum_string_helper.h"

#define SWAPCHAIN_CHECK(call, failedCall) \
	if (auto vkResult = call; vkResult != VK_SUCCESS) { \
        if (vkResult != VK_SUBOPTIMAL_KHR && vkResult != VK_ERROR_OUT_OF_DATE_KHR) { \
			errs("{} Failed. Vulkan Error {}", #call, string_VkResult(vkResult)); \
        } \
		failedCall; \
    }

static CVulkanResourceManager gSwapchainResourceManager;

static CVulkanResourceManager gFrameResourceManager;

void SSwapchainImage::destroy() {
	vkDestroyImageView(CRenderer::device(), mImageView, nullptr);
}

SSwapchain::SSwapchain(const vkb::Result<vkb::Swapchain>& inSwapchainBuilder) {
	//store swapchain and its related images
	mSwapchain = inSwapchainBuilder.value();

	const std::vector<VkImage> images = mSwapchain.get_images().value();
	const std::vector<VkImageView> imageViews = mSwapchain.get_image_views().value();

	VkSemaphoreCreateInfo semaphoreCreateInfo = CVulkanInfo::createSemaphoreInfo();

	// Allocate render semaphores
	mSwapchainRenderSemaphores.resize(images.size());
	mSwapchainImages.resize(images.size());

	for (uint32 i = 0; i < (uint32)images.size(); ++i) { //TODO: these cause vkDestroyImage errors
		gSwapchainResourceManager.create(mSwapchainRenderSemaphores[i], semaphoreCreateInfo);
		gSwapchainResourceManager.create(mSwapchainImages[i]);
		mSwapchainImages[i]->mImage = images[i];
		mSwapchainImages[i]->mImageView = imageViews[i];
		mSwapchainImages[i]->mBindlessAddress = i;
	}
}

CVulkanSwapchain::CVulkanSwapchain() {
	// Create one fence to control when the gpu has finished rendering the frame,
	// And 2 semaphores to synchronize rendering with swapchain
	VkFenceCreateInfo fenceCreateInfo = CVulkanInfo::createFenceInfo(VK_FENCE_CREATE_SIGNALED_BIT);
	VkSemaphoreCreateInfo semaphoreCreateInfo = CVulkanInfo::createSemaphoreInfo();

	for (auto& [mSwapchainSemaphore, mRenderSemaphore, mRenderFence, mPresentFence] : m_Frames) {

		gFrameResourceManager.create(mRenderFence, fenceCreateInfo);
		//gFrameResourceManager.createDestroyable(mPresentFence, fenceCreateInfo);

		gFrameResourceManager.create(mSwapchainSemaphore, semaphoreCreateInfo);
		//gFrameResourceManager.createDestroyable(mRenderSemaphore, semaphoreCreateInfo);
	}
}

void CVulkanSwapchain::init(const VkSwapchainKHR oldSwapchain, const bool inUseVSync) {
	mFormat = VK_FORMAT_R8G8B8A8_UNORM;

	m_VSync = inUseVSync;

	const auto& vkbSwapchain = vkb::SwapchainBuilder{CRenderer::device()}
		.set_old_swapchain(oldSwapchain)
		.set_desired_format(VkSurfaceFormatKHR{ .format = mFormat, .colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR })
		.set_desired_present_mode(m_VSync ? VK_PRESENT_MODE_FIFO_KHR : VK_PRESENT_MODE_IMMEDIATE_KHR)
		.set_desired_extent(CEngineViewport::get().mExtent.x, CEngineViewport::get().mExtent.y)
		.add_image_usage_flags(VK_IMAGE_USAGE_TRANSFER_DST_BIT)
		.build();

	if (!vkbSwapchain.has_value()) {
		errs("Swapchain failed to create. Vulkan Error: {}", string_VkResult(vkbSwapchain.full_error().vk_result));
	}

	// Flush any resources that may have been allocated
	gSwapchainResourceManager.flush();

	//Initialize internal swapchain
	gSwapchainResourceManager.create(mSwapchain, vkbSwapchain);
}

void CVulkanSwapchain::recreate(bool inUseVSync) {

	init(mSwapchain->mSwapchain, inUseVSync);

	m_Dirty = false;
}

void CVulkanSwapchain::destroy() {
	gSwapchainResourceManager.flush();
	gFrameResourceManager.flush();
}

SSwapchainImage* CVulkanSwapchain::getSwapchainImage(const uint32 inCurrentFrameIndex) {
	ZoneScoped;
	std::string zoneName("Swapchain Image Acquire");
	if (m_VSync) zoneName.append(" (VSync)");
	ZoneName(zoneName.c_str(), zoneName.size());

	uint32 swapchainImageIndex = 0;
	SWAPCHAIN_CHECK(vkAcquireNextImageKHR(CRenderer::device(), mSwapchain->mSwapchain, 1000000000, *m_Frames[inCurrentFrameIndex].mSwapchainSemaphore, nullptr, &swapchainImageIndex), setDirty());

	return mSwapchain->mSwapchainImages[swapchainImageIndex];
}

bool CVulkanSwapchain::wait(const uint32 inCurrentFrameIndex) const {
	ZoneScoped;
	std::string zoneName("Swapchain Wait");
	if (m_VSync) zoneName.append(" (VSync)");
	ZoneName(zoneName.c_str(), zoneName.size());

	auto& frame = m_Frames[inCurrentFrameIndex];
	VK_CHECK(vkWaitForFences(CRenderer::device(), 1, &frame.mRenderFence->mFence, true, 1000000000));
	//VK_CHECK(vkWaitForFences(CRenderer::device(), 1, &frame.mPresentFence, true, 1000000000));

	return true;
}

void CVulkanSwapchain::reset(const uint32 inCurrentFrameIndex) const {
	ZoneScoped;
	std::string zoneName("Swapchain Reset");
	if (m_VSync) zoneName.append(" (VSync)");
	ZoneName(zoneName.c_str(), zoneName.size());

	auto& frame = m_Frames[inCurrentFrameIndex];
	VK_CHECK(vkResetFences(CRenderer::device(), 1, &frame.mRenderFence->mFence));
	//VK_CHECK(vkResetFences(CRenderer::device(), 1, &frame.mPresentFence));
}

void CVulkanSwapchain::submit(const VkCommandBuffer inCmd, const VkQueue inGraphicsQueue, uint32 inCurrentFrameIndex, const uint32 inSwapchainImageIndex) {
	const auto& frame = m_Frames[inCurrentFrameIndex];

	ZoneScopedN("Present");

	// Submit
	{
		VkCommandBufferSubmitInfo cmdInfo = CVulkanInfo::submitCommandBufferInfo(inCmd);

		VkSemaphoreSubmitInfo waitInfo = CVulkanInfo::submitSemaphoreInfo(VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR, *frame.mSwapchainSemaphore);

		VkSemaphore renderSemaphore = *mSwapchain->mSwapchainRenderSemaphores[inSwapchainImageIndex];
		VkSemaphoreSubmitInfo signalInfo = CVulkanInfo::submitSemaphoreInfo(VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT, renderSemaphore);

		VkSubmitInfo2 submit = CVulkanInfo::submitInfo(&cmdInfo,&signalInfo,&waitInfo);
		VK_CHECK(vkQueueSubmit2(inGraphicsQueue, 1, &submit, *frame.mRenderFence));
	}

	// Present
	{
		/*VkSwapchainPresentFenceInfoEXT presentFenceInfo {
			.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_PRESENT_FENCE_INFO_EXT,
			.swapchainCount = 1,
			.pFences = &frame.mPresentFence
		};*/

		const auto presentInfo = VkPresentInfoKHR {
			.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
			.pNext = nullptr,
			.waitSemaphoreCount = 1,
			.pWaitSemaphores = &mSwapchain->mSwapchainRenderSemaphores[inSwapchainImageIndex]->mSemaphore,
			.swapchainCount = 1,
			.pSwapchains = &mSwapchain->mSwapchain.swapchain,
			.pImageIndices = &inSwapchainImageIndex,
		};

		SWAPCHAIN_CHECK(vkQueuePresentKHR(inGraphicsQueue, &presentInfo), setDirty());
	}
}
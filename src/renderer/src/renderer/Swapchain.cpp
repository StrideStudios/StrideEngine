#include "renderer/Swapchain.h"
#include "engine/Viewport.h"
#include "rendercore/VulkanDevice.h"
#include "rendercore/VulkanUtils.h"
#include "basic/Profiling.h"
#include "VkBootstrap.h"
#include "engine/Engine.h"
#include "tracy/Tracy.hpp"

#include "vulkan/vk_enum_string_helper.h"

#define SWAPCHAIN_CHECK(call, failedCall) \
	if (auto vkResult = call; vkResult != VK_SUCCESS) { \
        if (vkResult != VK_SUBOPTIMAL_KHR && vkResult != VK_ERROR_OUT_OF_DATE_KHR) { \
			errs("{} Failed. Vulkan Error {}", #call, string_VkResult(vkResult)); \
        } \
		failedCall; \
    }

void SSwapchainImage::destroy(const TShared<CVulkanDevice>& device) {
	vkDestroyImageView(device->getDevice().device, mImageView, nullptr);
}

void SSwapchain::init(const TShared<CVulkanDevice>& device, const vkb::Result<vkb::Swapchain>& inSwapchainBuilder) {
	//store swapchain and its related images
	mInternalSwapchain = std::make_shared<vkb::Swapchain>(inSwapchainBuilder.value());

	const std::vector<VkImage> images = mInternalSwapchain->get_images().value();
	const std::vector<VkImageView> imageViews = mInternalSwapchain->get_image_views().value();

	VkSemaphoreCreateInfo semaphoreCreateInfo = CVulkanInfo::createSemaphoreInfo();

	// Allocate render semaphores
	mSwapchainRenderSemaphores.resize(images.size(), [&](const size_t i){
		return TUnique<CSemaphore>{device, semaphoreCreateInfo};
	});

	mSwapchainImages.resize(images.size(), [&](const size_t i){
		TUnique<SSwapchainImage> image{};
		image->mImage = images[i];
		image->mImageView = imageViews[i];
		image->mBindlessAddress = i;
		return std::move(image);
	});
}

void SSwapchain::destroy(const TShared<CVulkanDevice>& device) {
	mSwapchainImages.forEachReverse([&device](size_t, const TUnique<SSwapchainImage>& object){
		object->destroy(device);
	});
	mSwapchainRenderSemaphores.forEachReverse([](size_t, const TUnique<CSemaphore>& object){
		object->destroy();
	});
	vkb::destroy_swapchain(*mInternalSwapchain);
}

CVulkanSwapchain::CVulkanSwapchain(const TShared<CRenderer>& renderer) {

	m_Frames.resize(renderer->getBufferingType().getFrameOverlap());

	// Create one fence to control when the gpu has finished rendering the frame,
	// And 2 semaphores to synchronize rendering with swapchain
	VkFenceCreateInfo fenceCreateInfo = CVulkanInfo::createFenceInfo(VK_FENCE_CREATE_SIGNALED_BIT);
	VkSemaphoreCreateInfo semaphoreCreateInfo = CVulkanInfo::createSemaphoreInfo();

	for (auto& [mSwapchainSemaphore, mRenderSemaphore, mRenderFence, mPresentFence] : m_Frames) {

		mRenderFence = TUnique<CFence>{renderer->device(), fenceCreateInfo};
		//mPresentFence = TUnique<CFence>{renderer->device(), fenceCreateInfo};

		mSwapchainSemaphore = TUnique<CSemaphore>{renderer->device(), semaphoreCreateInfo};
		//mRenderSemaphore = TUnique<CSemaphore>{renderer->device(), semaphoreCreateInfo};
	}

	create(renderer->device(), VK_NULL_HANDLE);
}

void CVulkanSwapchain::create(const TShared<CVulkanDevice>& device, const VkSwapchainKHR oldSwapchain, const bool inUseVSync) {
	mFormat = VK_FORMAT_R8G8B8A8_UNORM;

	m_VSync = inUseVSync;

	const auto& vkbSwapchain = vkb::SwapchainBuilder{device->getDevice()} //TODO: remove usage of device
		.set_old_swapchain(oldSwapchain)
		.set_desired_format(VkSurfaceFormatKHR{ .format = mFormat, .colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR })
		.set_desired_present_mode(m_VSync ? VK_PRESENT_MODE_FIFO_KHR : VK_PRESENT_MODE_IMMEDIATE_KHR)
		.set_desired_extent(CEngine::get()->getViewport()->mExtent.x, CEngine::get()->getViewport()->mExtent.y)
		.add_image_usage_flags(VK_IMAGE_USAGE_TRANSFER_DST_BIT)
		.build();

	if (!vkbSwapchain.has_value()) {
		errs("Swapchain failed to create. Vulkan Error: {}", string_VkResult(vkbSwapchain.full_error().vk_result));
	}

	// Flush any resources that may have been allocated
	if (mSwapchain) {
		mSwapchain->destroy(device);
	}

	//Initialize internal swapchain
	mSwapchain = TUnique<SSwapchain>{};
	mSwapchain->init(device, vkbSwapchain);
}

void CVulkanSwapchain::recreate(const TShared<CVulkanDevice>& device, const bool inUseVSync) {
	create(device, *mSwapchain->mInternalSwapchain, inUseVSync);
	clean();
}

void CVulkanSwapchain::destroy(const TShared<CVulkanDevice>& device) {
	if (mSwapchain) mSwapchain->destroy(device);
	for (auto& [mSwapchainSemaphore, mRenderSemaphore, mRenderFence, mPresentFence] : m_Frames) {
		mSwapchainSemaphore->destroy();
		mRenderFence->destroy();
	}
}

TUnique<SSwapchainImage>& CVulkanSwapchain::getSwapchainImage(const TShared<CVulkanDevice>& inDevice, const uint32 inCurrentFrameIndex) {
	ZoneScoped;
	std::string zoneName("Swapchain Image Acquire");
	if (m_VSync) zoneName.append(" (VSync)");
	ZoneName(zoneName.c_str(), zoneName.size());

	uint32 swapchainImageIndex = 0;
	SWAPCHAIN_CHECK(vkAcquireNextImageKHR(inDevice->getDevice().device, *mSwapchain->mInternalSwapchain, 1000000000, **m_Frames[inCurrentFrameIndex].mSwapchainSemaphore, nullptr, &swapchainImageIndex), setDirty());

	return mSwapchain->mSwapchainImages[swapchainImageIndex];
}

bool CVulkanSwapchain::wait(const TShared<CVulkanDevice>& inDevice, const uint32 inCurrentFrameIndex) const {
	ZoneScoped;
	std::string zoneName("Swapchain Wait");
	if (m_VSync) zoneName.append(" (VSync)");
	ZoneName(zoneName.c_str(), zoneName.size());

	auto& frame = m_Frames[inCurrentFrameIndex];
	VK_CHECK(vkWaitForFences(inDevice->getDevice().device, 1, &frame.mRenderFence->mFence, true, 1000000000));
	//VK_CHECK(vkWaitForFences(inDevice->getDevice().device, 1, &frame.mPresentFence, true, 1000000000));

	return true;
}

void CVulkanSwapchain::reset(const TShared<CVulkanDevice>& inDevice, const uint32 inCurrentFrameIndex) const {
	ZoneScoped;
	std::string zoneName("Swapchain Reset");
	if (m_VSync) zoneName.append(" (VSync)");
	ZoneName(zoneName.c_str(), zoneName.size());

	auto& frame = m_Frames[inCurrentFrameIndex];
	VK_CHECK(vkResetFences(inDevice->getDevice().device, 1, &frame.mRenderFence->mFence));
	//VK_CHECK(vkResetFences(inDevice->getDevice().device, 1, &frame.mPresentFence));
}

void CVulkanSwapchain::submit(const VkCommandBuffer inCmd, const VkQueue inGraphicsQueue, uint32 inCurrentFrameIndex, const uint32 inSwapchainImageIndex) {
	const auto& frame = m_Frames[inCurrentFrameIndex];

	ZoneScopedN("Present");

	// Submit
	{
		VkCommandBufferSubmitInfo cmdInfo = CVulkanInfo::submitCommandBufferInfo(inCmd);

		VkSemaphoreSubmitInfo waitInfo = CVulkanInfo::submitSemaphoreInfo(VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR, **frame.mSwapchainSemaphore);

		VkSemaphore renderSemaphore = **mSwapchain->mSwapchainRenderSemaphores[inSwapchainImageIndex];
		VkSemaphoreSubmitInfo signalInfo = CVulkanInfo::submitSemaphoreInfo(VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT, renderSemaphore);

		VkSubmitInfo2 submit = CVulkanInfo::submitInfo(&cmdInfo,&signalInfo,&waitInfo);
		VK_CHECK(vkQueueSubmit2(inGraphicsQueue, 1, &submit, **frame.mRenderFence));
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
			.pSwapchains = &mSwapchain->mInternalSwapchain->swapchain,
			.pImageIndices = &inSwapchainImageIndex,
		};

		SWAPCHAIN_CHECK(vkQueuePresentKHR(inGraphicsQueue, &presentInfo), setDirty());
	}
}
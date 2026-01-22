#include "VRI/VRISwapchain.h"

#include "SDL3/SDL_video.h"
#include "VkBootstrap.h"

#include "tracy/Tracy.hpp"
#include "VRI/VRICommands.h"

#include "vulkan/vulkan_core.h"
#include "vulkan/vk_enum_string_helper.h"

#define SWAPCHAIN_CHECK(call, failedCall) \
	if (auto vkResult = call; vkResult != VK_SUCCESS) { \
        if (vkResult != VK_SUBOPTIMAL_KHR && vkResult != VK_ERROR_OUT_OF_DATE_KHR) { \
			errs("{} Failed. Vulkan Error {}", #call, string_VkResult(vkResult)); \
        } \
		failedCall; \
    }

std::function<void()> SSwapchainImage::getDestroyer() {
	return [imageView = mImageView] {
		vkDestroyImageView(CVRI::get()->getDevice()->device, imageView, nullptr);
	};
}

void SSwapchain::init(const vkb::Result<vkb::Swapchain>& inSwapchainBuilder) {
	//store swapchain and its related images
	mInternalSwapchain = std::make_shared<vkb::Swapchain>(inSwapchainBuilder.value());

	const std::vector<VkImage> images = mInternalSwapchain->get_images().value();
	const std::vector<VkImageView> imageViews = mInternalSwapchain->get_image_views().value();

	constexpr VkSemaphoreCreateInfo semaphoreCreateInfo {
		.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0
   };

	// Allocate render semaphores
	mSwapchainRenderSemaphores.resize(images.size(), [&](const size_t i){
		return TUnique<CSemaphore>{semaphoreCreateInfo};
	});

	mSwapchainImages.resize(images.size(), [&](const size_t i){
		TUnique<SSwapchainImage> image{};
		image->mImage = images[i];
		image->mImageView = imageViews[i];
		image->mBindlessAddress = i;
		return std::move(image);
	});
}

void SSwapchain::destroy() {
	mSwapchainImages.forEachReverse([](size_t, TUnique<SSwapchainImage>& object){
		object.destroy();
	});
	mSwapchainRenderSemaphores.forEachReverse([](size_t, TUnique<CSemaphore>& object){
		object.destroy();
	});
	vkb::destroy_swapchain(*mInternalSwapchain);
}

CVRISwapchain::CVRISwapchain(SDL_Window* window): m_Window(window) {

	m_Frames.data().resize([](size_t) {
		return TUnique<FrameData>{};
	});

	// Create one fence to control when the gpu has finished rendering the frame,
	// And 2 semaphores to synchronize rendering with swapchain
	constexpr VkFenceCreateInfo fenceCreateInfo {
		.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
	   .pNext = nullptr,
	   .flags = VK_FENCE_CREATE_SIGNALED_BIT
   };

	constexpr VkSemaphoreCreateInfo semaphoreCreateInfo {
		.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0
	};

	m_Frames.data().forEach([&](size_t, const TUnique<FrameData>& data) {
		data->mRenderFence = TUnique<CFence>{fenceCreateInfo};
		//mPresentFence = TUnique<CFence>{fenceCreateInfo};

		data->mSwapchainSemaphore = TUnique<CSemaphore>{semaphoreCreateInfo};
		//mRenderSemaphore = TUnique<CSemaphore>{semaphoreCreateInfo};
	});

	create(VK_NULL_HANDLE);
}

void CVRISwapchain::create(const VkSwapchainKHR oldSwapchain, const bool inUseVSync) {
	mFormat = VK_FORMAT_R8G8B8A8_UNORM;

	m_VSync = inUseVSync;


	int width, height;
	SDL_GetWindowSize(m_Window, &width, &height);

	const auto& vkbSwapchain = vkb::SwapchainBuilder{*CVRI::get()->getDevice()} //TODO: remove usage of device
		.set_old_swapchain(oldSwapchain)
		.set_desired_format(VkSurfaceFormatKHR{ .format = mFormat, .colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR })
		.set_desired_present_mode(m_VSync ? VK_PRESENT_MODE_FIFO_KHR : VK_PRESENT_MODE_IMMEDIATE_KHR)
		.set_desired_extent(width, height)
		.add_image_usage_flags(VK_IMAGE_USAGE_TRANSFER_DST_BIT)
		.build();

	if (!vkbSwapchain.has_value()) {
		errs("Swapchain failed to create. Vulkan Error: {}", string_VkResult(vkbSwapchain.full_error().vk_result));
	}

	// Flush any resources that may have been allocated
	if (mSwapchain) {
		mSwapchain.destroy();
	}

	//Initialize internal swapchain
	mSwapchain = TUnique<SSwapchain>{};
	mSwapchain->init(vkbSwapchain);
}

void CVRISwapchain::recreate(const bool inUseVSync) {
	create(*mSwapchain->mInternalSwapchain, inUseVSync);
	clean();
}

void CVRISwapchain::destroy2() {
	if (mSwapchain) mSwapchain.destroy();
	m_Frames.data().forEach([](size_t, const TUnique<FrameData>& data) {
		data->mSwapchainSemaphore.destroy();
		data->mRenderFence.destroy();
	});
}

TUnique<SSwapchainImage>& CVRISwapchain::getSwapchainImage() {
	ZoneScoped;
	std::string zoneName("Swapchain Image Acquire");
	if (m_VSync) zoneName.append(" (VSync)");
	ZoneName(zoneName.c_str(), zoneName.size());

	uint32 swapchainImageIndex = 0;
	SWAPCHAIN_CHECK(vkAcquireNextImageKHR(CVRI::get()->getDevice()->device, *mSwapchain->mInternalSwapchain, 1000000000, *m_Frames.getFrame(m_Buffering.getFrameIndex())->mSwapchainSemaphore, nullptr, &swapchainImageIndex), setDirty());

	return mSwapchain->mSwapchainImages[swapchainImageIndex];
}

bool CVRISwapchain::wait() const {
	ZoneScoped;
	std::string zoneName("Swapchain Wait");
	if (m_VSync) zoneName.append(" (VSync)");
	ZoneName(zoneName.c_str(), zoneName.size());

	auto& frame = m_Frames.getFrame(m_Buffering.getFrameIndex());
	VK_CHECK(vkWaitForFences(CVRI::get()->getDevice()->device, 1, &frame->mRenderFence->mFence, true, 1000000000));
	//VK_CHECK(vkWaitForFences(inDevice->getDevice().device, 1, &frame.mPresentFence, true, 1000000000));

	return true;
}

void CVRISwapchain::reset() const {
	ZoneScoped;
	std::string zoneName("Swapchain Reset");
	if (m_VSync) zoneName.append(" (VSync)");
	ZoneName(zoneName.c_str(), zoneName.size());

	auto& frame = m_Frames.getFrame(m_Buffering.getFrameIndex());
	VK_CHECK(vkResetFences(CVRI::get()->getDevice()->device, 1, &frame->mRenderFence->mFence));
	//VK_CHECK(vkResetFences(inDevice->getDevice().device, 1, &frame.mPresentFence));
}

void CVRISwapchain::submit(const TFrail<CVRICommands>& inCmd, const VkQueue inGraphicsQueue, const uint32 inSwapchainImageIndex) {
	const auto& frame = m_Frames.getFrame(m_Buffering.getFrameIndex());

	ZoneScopedN("Present");

	// Submit
	{
		VkCommandBufferSubmitInfo cmdInfo = inCmd->submitInfo();

		VkSemaphoreSubmitInfo waitInfo {
			.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
		   .pNext = nullptr,
		   .semaphore = *frame->mSwapchainSemaphore,
		   .value = 1,
		   .stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR,
		   .deviceIndex = 0
		};

		const VkSemaphore renderSemaphore = *mSwapchain->mSwapchainRenderSemaphores[inSwapchainImageIndex];
		VkSemaphoreSubmitInfo signalInfo {
			.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
		   .pNext = nullptr,
		   .semaphore = renderSemaphore,
		   .value = 1,
		   .stageMask = VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT,
		   .deviceIndex = 0
		};

		const VkSubmitInfo2 submit {
			.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
		   .pNext = nullptr,

		   .waitSemaphoreInfoCount = static_cast<uint32>(1),
		   .pWaitSemaphoreInfos = &waitInfo,

		   .commandBufferInfoCount = 1,
		   .pCommandBufferInfos = &cmdInfo,

		   .signalSemaphoreInfoCount = static_cast<uint32>(1),
		   .pSignalSemaphoreInfos = &signalInfo
		};
		VK_CHECK(vkQueueSubmit2(inGraphicsQueue, 1, &submit, *frame->mRenderFence));
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

	//increase the number of frames drawn
	m_Buffering.incrementFrame();
}
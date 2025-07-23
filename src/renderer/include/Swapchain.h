#pragma once

#include <iostream>
#include <vector>
#include <vulkan/vulkan_core.h>

#include "Common.h"
#include "VkBootstrap.h"
#include "VulkanUtils.h"

struct SFrameData;
class CVulkanDevice;

#define SWAPCHAIN_CHECK(call, failedCall) \
	if (auto vkResult = call; vkResult != VK_SUCCESS) { \
        if (vkResult != VK_SUBOPTIMAL_KHR && vkResult != VK_ERROR_OUT_OF_DATE_KHR) { \
			err("{} Failed. Vulkan Error {}", #call, string_VkResult(vkResult)); \
        } \
    failedCall; \
    }

class CSwapchain {

public:

	struct SFrameData {
		VkSemaphore mSwapchainSemaphore = nullptr;
		VkSemaphore mRenderSemaphore = nullptr;

		VkFence mRenderFence = nullptr;
		VkFence mPresentFence = nullptr;
	};

	CSwapchain();

	no_discard bool isDirty() const {
		return m_Dirty;
	}

	void setDirty() {
		m_Dirty = true;
	}

	operator VkSwapchainKHR() const {
		return mSwapchain;
	}

	void recreate(bool inUseVSync = true);

	void cleanup();

	std::pair<VkImage, uint32> getSwapchainImage(uint32 inCurrentFrameIndex);

	void wait(uint32 inCurrentFrameIndex) const;

	void reset(uint32 inCurrentFrameIndex) const;

	void submit(VkCommandBuffer inCmd, VkQueue inGraphicsQueue, uint32 inCurrentFrameIndex, uint32 inSwapchainImageIndex);

	vkb::Swapchain mSwapchain;

	std::vector<VkImage> mSwapchainImages;

	std::vector<VkImageView> mSwapchainImageViews;

	// TODO: Needed if Maintenence1 is not available, should union
	// TODO: this means i probably need some sort of platform info or something
	//std::vector<VkSemaphore> mSwapchainImageViews;

	VkFormat mFormat;

private:

	SFrameData m_Frames[gFrameOverlap];

	bool m_Dirty = false;
};

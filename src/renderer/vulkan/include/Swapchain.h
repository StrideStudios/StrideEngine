#pragma once

#include <vector>
#include <vulkan/vulkan_core.h>

#include "Common.h"
#include "ResourceManager.h"

// Forward declare vkb types
namespace vkb {
	struct Swapchain;
}

class CVulkanDevice;

class CSwapchain {

public:

	struct FrameData {
		VkSemaphore mSwapchainSemaphore = nullptr;
		VkSemaphore mRenderSemaphore = nullptr;

		VkFence mRenderFence = nullptr;
		VkFence mPresentFence = nullptr;
	};

	CSwapchain();

	~CSwapchain();

	no_discard bool isDirty() const {
		return m_Dirty;
	}

	void setDirty() {
		m_Dirty = true;
	}

	operator VkSwapchainKHR() const;

	void init(VkSwapchainKHR oldSwapchain = VK_NULL_HANDLE, bool inUseVSync = true);

	void recreate(bool inUseVSync = true);

	void cleanup();

	std::tuple<VkImage, VkImageView, uint32> getSwapchainImage(uint32 inCurrentFrameIndex);

	no_discard bool wait(uint32 inCurrentFrameIndex) const;

	void reset(uint32 inCurrentFrameIndex) const;

	void submit(VkCommandBuffer inCmd, VkQueue inGraphicsQueue, uint32 inCurrentFrameIndex, uint32 inSwapchainImageIndex);

	std::unique_ptr<vkb::Swapchain> mSwapchain;

	std::vector<VkImage> mSwapchainImages;

	std::vector<VkImageView> mSwapchainImageViews;

	// TODO: Needed if Maintenence1 is not available, should union
	// TODO: this means i probably need some sort of platform info or something
	std::vector<VkSemaphore> mSwapchainRenderSemaphores;

	VkFormat mFormat;

private:

	CResourceManager m_ResourceManager;

	FrameData m_Frames[gFrameOverlap];

	bool m_VSync = true;

	bool m_Dirty = false;
};

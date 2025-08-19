#pragma once

#include <vector>
#include <vulkan/vulkan_core.h>
#include <VkBootstrap.h>

#include "VulkanResources.h"

// Forward declare vkb types
namespace vkb {
	struct Swapchain;
}

class CVulkanDevice;

struct SSwapchain final : IDestroyable {

	SSwapchain(const vkb::Result<vkb::Swapchain>& inSwapchainBuilder);

	virtual void destroy() override {
		vkb::destroy_swapchain(mSwapchain);
	}

	vkb::Swapchain mSwapchain;

	std::vector<VkImage> mSwapchainImages{};

	std::vector<CImageView*> mSwapchainImageViews{};

	// TODO: Needed if Maintenence1 is not available, should union
	// TODO: this means i probably need some sort of platform info or something
	std::vector<CSemaphore*> mSwapchainRenderSemaphores{};
};

class CSwapchain : public IInitializable<VkSwapchainKHR, bool>, public IDestroyable {

public:

	struct FrameData {
		CSemaphore* mSwapchainSemaphore = nullptr;
		CSemaphore* mRenderSemaphore = nullptr;

		CFence* mRenderFence = nullptr;
		CFence* mPresentFence = nullptr;
	};

	CSwapchain();

	no_discard bool isDirty() const {
		return m_Dirty;
	}

	void setDirty() {
		m_Dirty = true;
	}

	virtual void init(VkSwapchainKHR oldSwapchain = VK_NULL_HANDLE, bool inUseVSync = true) override;

	void recreate(bool inUseVSync = true);

	virtual void destroy() override;

	std::tuple<VkImage, VkImageView, uint32> getSwapchainImage(uint32 inCurrentFrameIndex);

	no_discard bool wait(uint32 inCurrentFrameIndex) const;

	void reset(uint32 inCurrentFrameIndex) const;

	void submit(VkCommandBuffer inCmd, VkQueue inGraphicsQueue, uint32 inCurrentFrameIndex, uint32 inSwapchainImageIndex);

	SSwapchain* mSwapchain = nullptr;

	VkFormat mFormat = VK_FORMAT_UNDEFINED;

private:

	FrameData m_Frames[gFrameOverlap];

	bool m_VSync = true;

	bool m_Dirty = false;
};

#pragma once

#include <vector>
#include <vulkan/vulkan_core.h>

#include "VulkanResources.h"

// Forward declare vkb types
namespace vkb {
	struct Swapchain;
	template <typename T> class Result;
}

class CVulkanDevice;

struct SSwapchainImage : SImage_T {
	virtual void destroy() override;
};

struct SSwapchain final : SObject, TInitializable<const vkb::Result<vkb::Swapchain>&>, IDestroyable {

	REGISTER_STRUCT(SSwapchain)

	EXPORT virtual void init(const vkb::Result<vkb::Swapchain>& inSwapchainBuilder) override;

	EXPORT virtual void destroy() override;

	std::unique_ptr<vkb::Swapchain> mInternalSwapchain;

	std::vector<SSwapchainImage*> mSwapchainImages{};

	// TODO: Needed if Maintenence1 is not available, should union
	// TODO: this means i probably need some sort of platform info or something
	std::vector<CSemaphore*> mSwapchainRenderSemaphores{};
};

class CVulkanSwapchain : public CSwapchain, public IInitializable, public IDestroyable {

	REGISTER_CLASS(CVulkanSwapchain)

public:

	struct FrameData {
		CSemaphore* mSwapchainSemaphore = nullptr;
		CSemaphore* mRenderSemaphore = nullptr;

		CFence* mRenderFence = nullptr;
		CFence* mPresentFence = nullptr;
	};

	EXPORT CVulkanSwapchain();

	no_discard virtual bool isDirty() const override {
		return m_Dirty;
	}

	virtual void setDirty() override {
		m_Dirty = true;
	}

	virtual void init() override {
		init(VK_NULL_HANDLE);
	}

	EXPORT virtual void init(VkSwapchainKHR oldSwapchain, bool inUseVSync = true);

	EXPORT void recreate(bool inUseVSync = true);

	EXPORT virtual void destroy() override;

	EXPORT SSwapchainImage* getSwapchainImage(uint32 inCurrentFrameIndex);

	no_discard EXPORT bool wait(uint32 inCurrentFrameIndex) const;

	EXPORT void reset(uint32 inCurrentFrameIndex) const;

	EXPORT void submit(VkCommandBuffer inCmd, VkQueue inGraphicsQueue, uint32 inCurrentFrameIndex, uint32 inSwapchainImageIndex);

	SSwapchain* mSwapchain = nullptr;

	VkFormat mFormat = VK_FORMAT_UNDEFINED;

private:

	FrameData m_Frames[gFrameOverlap];

	bool m_VSync = true;

	bool m_Dirty = true;
};

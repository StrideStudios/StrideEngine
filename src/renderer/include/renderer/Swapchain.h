#pragma once

#include <vector>
#include <vulkan/vulkan_core.h>

#include "rendercore/VulkanResources.h"
#include "sstl/Vector.h"

// Forward declare vkb types
namespace vkb {
	struct Swapchain;
	template <typename T> class Result;
}

class CVulkanDevice;

struct SSwapchainImage : SImage_T {
	EXPORT virtual void destroy() override;
};

struct SSwapchain final : SObject, TInitializable<const vkb::Result<vkb::Swapchain>&>, IDestroyable {

	REGISTER_STRUCT(SSwapchain, SObject)

	EXPORT virtual void init(const vkb::Result<vkb::Swapchain>& inSwapchainBuilder) override;

	EXPORT virtual void destroy() override;

	std::shared_ptr<vkb::Swapchain> mInternalSwapchain;

	TVector<TUnique<SSwapchainImage>> mSwapchainImages;

	// TODO: Needed if Maintenence1 is not available, should union
	// TODO: this means i probably need some sort of platform info or something
	TVector<TUnique<CSemaphore>> mSwapchainRenderSemaphores;
};

//TODO: does not need to be SObject derived
class CVulkanSwapchain : public CSwapchain, public IInitializable, public IDestroyable {

	REGISTER_CLASS(CVulkanSwapchain, CSwapchain)

public:

	struct FrameData {
		CSemaphore* mSwapchainSemaphore = nullptr;
		CSemaphore* mRenderSemaphore = nullptr;

		CFence* mRenderFence = nullptr;
		CFence* mPresentFence = nullptr;
	};

	CVulkanSwapchain() = default;
	EXPORT CVulkanSwapchain(const TShared<CRenderer>& renderer);

	virtual void init() override {
		init(VK_NULL_HANDLE);
	}

	EXPORT virtual void init(VkSwapchainKHR oldSwapchain, bool inUseVSync = true);

	EXPORT void recreate(bool inUseVSync = true);

	EXPORT virtual void destroy() override;

	EXPORT TUnique<SSwapchainImage>& getSwapchainImage(const TShared<CVulkanDevice>& inDevice, uint32 inCurrentFrameIndex);

	no_discard EXPORT bool wait(const TShared<CVulkanDevice>& inDevice, uint32 inCurrentFrameIndex) const;

	EXPORT void reset(const TShared<CVulkanDevice>& inDevice, uint32 inCurrentFrameIndex) const;

	EXPORT void submit(VkCommandBuffer inCmd, VkQueue inGraphicsQueue, uint32 inCurrentFrameIndex, uint32 inSwapchainImageIndex);

	TUnique<SSwapchain> mSwapchain = nullptr;

	VkFormat mFormat = VK_FORMAT_UNDEFINED;

private:

	std::vector<FrameData> m_Frames;

	bool m_VSync = true;
};

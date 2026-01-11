#pragma once

#include <vector>
#include <vulkan/vulkan_core.h>

#include "rendercore/Renderer.h"
#include "rendercore/VulkanResources.h"
#include "sstl/Vector.h"

class CVRICommands;

// Forward declare vkb types
namespace vkb {
	struct Swapchain;
	template <typename T> class Result;
}

struct SSwapchainImage : SVRIImage {
	EXPORT void destroy();
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
class CVulkanSwapchain : public CSwapchain {

	REGISTER_CLASS(CVulkanSwapchain, CSwapchain)

public:

	struct FrameData {
		TUnique<CSemaphore> mSwapchainSemaphore = nullptr;
		TUnique<CSemaphore> mRenderSemaphore = nullptr;

		TUnique<CFence> mRenderFence = nullptr;
		TUnique<CFence> mPresentFence = nullptr;
	};

	CVulkanSwapchain() = default;
	EXPORT CVulkanSwapchain(const TFrail<CRenderer>& renderer);

	EXPORT void create(VkSwapchainKHR oldSwapchain, bool inUseVSync = true);
	EXPORT void recreate(bool inUseVSync = true);

	EXPORT virtual void destroy();

	EXPORT TUnique<SSwapchainImage>& getSwapchainImage(uint32 inCurrentFrameIndex);

	no_discard EXPORT bool wait(uint32 inCurrentFrameIndex) const;

	EXPORT void reset(uint32 inCurrentFrameIndex) const;

	EXPORT void submit(const TFrail<CVRICommands>& inCmd, VkQueue inGraphicsQueue, uint32 inCurrentFrameIndex, uint32 inSwapchainImageIndex);

	TUnique<SSwapchain> mSwapchain = nullptr;

	VkFormat mFormat = VK_FORMAT_UNDEFINED;

private:

	std::vector<FrameData> m_Frames;

	bool m_VSync = true;
};

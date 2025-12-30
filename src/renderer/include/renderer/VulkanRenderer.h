#pragma once

#include <vulkan/vulkan_core.h>
#include <functional>
#include <memory>

#include "rendercore/Renderer.h"
#include "rendercore/VulkanResources.h"
#include "sstl/Threading.h"

class CEngineTextures;

// Forward declare vkb types
namespace vkb {
	struct Instance;
	struct Device;
	struct PhysicalDevice;
}

namespace tracy {
	class VkCtx;
}

class CVulkanDevice;
class CVulkanInstance;

struct SUploadContext {
	TUnique<CCommandPool> mCommandPool = nullptr;
	SCommandBuffer mCommandBuffer{};
	TUnique<CFence> mUploadFence = nullptr;
};

struct CVulkanSurface {

	EXPORT CVulkanSurface(struct SDL_Window* window, const TFrail<CVulkanInstance>& instance);
	EXPORT ~CVulkanSurface();

	VkSurfaceKHR mVkSurface = nullptr;

private:

	TFrail<CVulkanInstance> mInstance = nullptr;

};

class CVulkanRenderer : public CRenderer {

	REGISTER_CLASS(CVulkanRenderer, CRenderer)

public:

	struct SceneData {
		Matrix4f mViewProj;
		Vector2f mScreenSize;
		Vector2f mInvScreenSize;
		Vector4f mAmbientColor;
		Vector4f mSunlightDirection; // w for sun power
		Vector4f mSunlightColor;
	};

	struct FrameData {

		EXPORT FrameData(const TFrail<CVulkanDevice>& device, const VkCommandPoolCreateInfo& info);

		EXPORT ~FrameData();

		TUnique<CCommandPool> mCommandPool = nullptr;
		SCommandBuffer mMainCommandBuffer{};

		tracy::VkCtx* mTracyContext;
	};

	EXPORT CVulkanRenderer();

	EXPORT virtual void immediateSubmit(std::function<void(SCommandBuffer& cmd)>&& function) override;

	EXPORT virtual void init() override;

	EXPORT virtual void destroy() override;

	virtual IBuffering& getBufferingType() override { return mBuffering; }

	EXPORT virtual TShared<CSwapchain> getSwapchain() override;

	// Draw to the screen
	EXPORT virtual void render(SRendererInfo& info) override;

	no_discard EXPORT virtual bool wait() override;

	EXPORT virtual TShared<CVulkanDevice> device() override;

	EXPORT virtual TShared<CVulkanInstance> instance() override;

	// Tell children to render
	virtual void render(VkCommandBuffer cmd) {};

	TShared<CVulkanInstance> mInstance = nullptr;

	// Vulkan window surface
	TShared<CVulkanSurface> mVkSurface = nullptr;

	TShared<CVulkanDevice> mDevice = nullptr;

	TThreadSafe<SUploadContext> mUploadContext;

	CDoubleBuffering<FrameData> mBuffering{};

	TShared<CVulkanAllocator> mAllocator = nullptr;

	// Stores textures used internally by the engine
	TShared<CEngineTextures> mEngineTextures = nullptr;

	SStaticBuffer<VMA_MEMORY_USAGE_CPU_TO_GPU, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, sizeof(SceneData), 1> mSceneBuffer{"Scene Buffer"};

	SceneData mSceneData{};

	bool mVSync;

};

class CNullRenderer final : public CVulkanRenderer {

public:

	virtual void render(VkCommandBuffer cmd) override;
};
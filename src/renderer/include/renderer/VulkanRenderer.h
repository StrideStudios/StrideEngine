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
	CFence* mUploadFence = nullptr;
	CCommandPool* mCommandPool = nullptr;
	SCommandBuffer mCommandBuffer{};
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
		CCommandPool* mCommandPool = nullptr;
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

	// Stores textures used internally by the engine
	TShared<CEngineTextures> mEngineTextures = nullptr;

	//
	// Rendering Utils
	//

	TShared<CVulkanDevice> mDevice = nullptr;

	TShared<CVulkanInstance> mInstance = nullptr;

	TShared<CVulkanAllocator> mAllocator = nullptr;

	// Vulkan window surface
	VkSurfaceKHR mVkSurface = nullptr;

	bool mVSync;

	CDoubleBuffering<FrameData> mBuffering{};

	TThreadSafe<SUploadContext> mUploadContext;

	//
	// Scene Data
	//

	SceneData mSceneData{};

	SStaticBuffer<VMA_MEMORY_USAGE_CPU_TO_GPU, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, sizeof(SceneData), 1> mSceneBuffer{"Scene Buffer"};

	//
	// Objects
	//

	std::vector<std::shared_ptr<class CSceneObject>> mObjects{};

};

class CNullRenderer final : public CVulkanRenderer {

public:

	virtual void render(VkCommandBuffer cmd) override;
};
#pragma once

#include <vulkan/vulkan_core.h>
#include <functional>
#include <memory>

#include "rendercore/Renderer.h"
#include "rendercore/VulkanResources.h"
#include "sstl/Threading.h"
#include "VRI/VRISwapchain.h"

class CVRICommands;
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
	TUnique<CVRICommands> mCommands = nullptr;
	TUnique<CFence> mUploadFence = nullptr;
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

		EXPORT FrameData(const VkCommandPoolCreateInfo& info);

		EXPORT ~FrameData();

		TUnique<CCommandPool> mCommandPool = nullptr;
		TUnique<CVRICommands> mCommands = nullptr;

		tracy::VkCtx* mTracyContext;
	};

	EXPORT CVulkanRenderer();

	EXPORT virtual void immediateSubmit(std::function<void(TFrail<CVRICommands>)>&& function) override;

	EXPORT virtual void init() override;

	EXPORT virtual void destroy() override;

	// Draw to the screen
	EXPORT virtual void render(SRendererInfo& info) override;

	no_discard EXPORT virtual bool wait() override;

	// Tell children to render
	virtual void render(const TFrail<CVRICommands>& cmd) {};

	TThreadSafe<SUploadContext> mUploadContext;

	CVRISwapchain::Buffering::Resource<TUnique<FrameData>> mFrameData{};

	// Stores textures used internally by the engine
	TShared<CEngineTextures> mEngineTextures = nullptr;

	SStaticBuffer<VMA_MEMORY_USAGE_CPU_TO_GPU, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, sizeof(SceneData), 1> mSceneBuffer{"Scene Buffer"};

	SceneData mSceneData{};

	bool mVSync;

};

class CNullRenderer final : public CVulkanRenderer {

public:

	virtual void render(const TFrail<CVRICommands>& cmd) override;
};
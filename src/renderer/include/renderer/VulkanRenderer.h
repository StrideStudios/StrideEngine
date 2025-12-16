#pragma once

#include <vulkan/vulkan_core.h>
#include <functional>
#include <memory>
#include <mutex>

#include "rendercore/Renderer.h"
#include "rendercore/VulkanResources.h"

// Forward declare vkb types
namespace vkb {
	struct Instance;
	struct Device;
	struct PhysicalDevice;
}

namespace tracy {
	class VkCtx;
}

class CEngineTextures;
class CVulkanDevice;
class CVulkanInstance;

struct SUploadContext {
	std::mutex mMutex;
	CFence* mUploadFence;
	CCommandPool* mCommandPool;
	SCommandBuffer mCommandBuffer{};
};

class CVulkanRenderer : public CRenderer {

	REGISTER_CLASS(CVulkanRenderer, CRenderer)

	//MAKE_SINGLETON(CVulkanRenderer)

public:

	EXPORT static CVulkanRenderer* get();

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

	EXPORT virtual CSwapchain* getSwapchain() override;

	// Draw to the screen
	EXPORT virtual void render() override;

	no_discard EXPORT virtual bool wait() override;

	// Tell children to render
	virtual void render(VkCommandBuffer cmd) {};

	// Stores textures used internally by the engine
	CEngineTextures* mEngineTextures = nullptr;

	//
	// Rendering Utils
	//

	// Vulkan window surface
	VkSurfaceKHR mVkSurface;

	bool mVSync;

	CDoubleBuffering<FrameData> mBuffering;

	SUploadContext mUploadContext;

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
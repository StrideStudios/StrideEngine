#pragma once

#include <vulkan/vulkan_core.h>
#include <functional>
#include <memory>
#include <mutex>

#include "Renderer.h"
#include "VulkanResourceManager.h"

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

class EXPORT CVulkanRenderer : public CRenderer {

public:

	static CVulkanRenderer* get();

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

		/*
		 * A resource allocator that is persistent for a single frame
		 * Good for data that only needs to exist for a single frame
		 */
		CVulkanResourceManager mFrameResourceManager;

		tracy::VkCtx* mTracyContext;
	};

	CVulkanRenderer();

	virtual void immediateSubmit(std::function<void(SCommandBuffer& cmd)>&& function) override;

	virtual void init() override;

	virtual void destroy() override;

	force_inline uint32 getFrameNumber() const { return mFrameNumber; }

	force_inline uint32 getFrameIndex() const { return mFrameNumber % gFrameOverlap; }

	force_inline FrameData& getCurrentFrame() { return mFrames[getFrameIndex()]; }

	force_inline const FrameData& getCurrentFrame() const { return mFrames[getFrameIndex()]; }

	virtual CInstance* getInstance() override;

	virtual CDevice* getDevice() override;

	virtual CSwapchain* getSwapchain() override;

	// Draw to the screen
	virtual void render() override;

	no_discard virtual bool wait() override;

	// Tell children to render
	virtual void render(VkCommandBuffer cmd) {};

	// Stores textures used internally by the engine
	CEngineTextures* mEngineTextures = nullptr;

	//
	// Rendering Utils
	//

	// Vulkan window surface
	VkSurfaceKHR mVkSurface;

	CVulkanInstance* m_Instance = nullptr;

    CVulkanDevice* m_Device = nullptr;

	bool mVSync;

	uint64 mFrameNumber = 0;

	FrameData mFrames[gFrameOverlap]{};

	SUploadContext mUploadContext;

	//
	// Scene Data
	//

	SceneData mSceneData{};

	SBuffer_T* mSceneDataBuffer{};

	//
	// Objects
	//

	std::vector<std::shared_ptr<class CSceneObject>> mObjects{};

	//
	// Passes
	//

	class CMeshPass* mBasePass = nullptr;

	class CSpritePass* mSpritePass = nullptr;

};

class CNullRenderer final : public CVulkanRenderer {

public:

	virtual void render(VkCommandBuffer cmd) override;
};
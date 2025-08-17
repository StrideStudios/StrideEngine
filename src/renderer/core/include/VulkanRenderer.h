#pragma once

#include <vulkan/vulkan_core.h>
#include <functional>
#include <memory>
#include <mutex>

#include "ModuleManager.h"
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
struct SVulkanInstance;

struct SUploadContext {
	std::mutex mMutex;
	CFence* mUploadFence;
	CCommandPool* mCommandPool;
	VkCommandBuffer mCommandBuffer{};
};

//TODO: seperate class (all dll should have sections with a _API
class EXPORT CVulkanRendererModule final : public CRendererModule {

	ADD_MODULE(CVulkanRendererModule, "renderer")

public:

	virtual void init() override;

	virtual void destroy() override;

	virtual void render() override;

	virtual bool wait() override;

	class CVulkanRenderer* mRenderer = nullptr;

};

class CVulkanRenderer : public IInitializable<>, public IDestroyable {

public:

	static CVulkanRenderer& get() {
		return *CVulkanRendererModule::get().mRenderer;
	}

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
		VkCommandBuffer mMainCommandBuffer = nullptr;

		/*
		 * A resource allocator that is persistent for a single frame
		 * Good for data that only needs to exist for a single frame
		 */
		CVulkanResourceManager mFrameResourceManager;

		tracy::VkCtx* mTracyContext;
	};

	CVulkanRenderer();

	static const vkb::Instance& instance();

	static const vkb::Device& device();

	static const vkb::PhysicalDevice& physicalDevice();

	void immediateSubmit(std::function<void(VkCommandBuffer cmd)>&& function);

	virtual void init() override;

	virtual void destroy() override;

	force_inline uint32 getFrameNumber() const { return mFrameNumber; }

	force_inline uint32 getFrameIndex() const { return mFrameNumber % gFrameOverlap; }

	force_inline FrameData& getCurrentFrame() { return mFrames[getFrameIndex()]; }

	force_inline const FrameData& getCurrentFrame() const { return mFrames[getFrameIndex()]; }

	// Draw to the screen
	void render();

	// Tell children to render
	virtual void render(VkCommandBuffer cmd) {};

	no_discard bool wait();

	/*
	 * This resource allocator is flushed when the renderer is destroyed
	 * This is useful for any objects that need a persistent lifetime
	 */
	CVulkanResourceManager mGlobalResourceManager;

	// Stores textures used internally by the engine
	CEngineTextures* mEngineTextures = nullptr;

	//
	// Rendering Utils
	//

	// Vulkan window surface
	VkSurfaceKHR mVkSurface;

	SVulkanInstance* m_Instance = nullptr;

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
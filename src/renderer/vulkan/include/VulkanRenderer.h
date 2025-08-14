#pragma once

#include <vulkan/vulkan_core.h>
#include <functional>
#include <memory>
#include <mutex>

#include "Common.h"
#include "ResourceManager.h"
#include "VulkanResourceManager.h"

namespace tracy {
	class VkCtx;
}

class CEngineTextures;
class CVulkanDevice;

struct SUploadContext {
	std::mutex mMutex;
	CFence* mUploadFence;
	CCommandPool* mCommandPool;
	VkCommandBuffer mCommandBuffer{};
};

class CVulkanRenderer : public IInitializable<>, public IDestroyable {

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
		VkCommandBuffer mMainCommandBuffer = nullptr;

		/*
		 * A resource allocator that is persistent for a single frame
		 * Good for data that only needs to exist for a single frame
		 */
		CVulkanResourceManager mFrameResourceManager;

		tracy::VkCtx* mTracyContext;
	};

	CVulkanRenderer();

	static void immediateSubmit(std::function<void(VkCommandBuffer cmd)>&& function);

	virtual void init() override;

	virtual void destroy() override;

	force_inline uint32 getFrameNumber() const { return mFrameNumber; }

	force_inline uint32 getFrameIndex() const { return mFrameNumber % gFrameOverlap; }

	force_inline FrameData& getCurrentFrame() { return mFrames[getFrameIndex()]; }

	force_inline const FrameData& getCurrentFrame() const { return mFrames[getFrameIndex()]; }

	// Draw to the screen
	void draw();

	// Tell children to render
	virtual void render(VkCommandBuffer cmd) = 0;

	no_discard bool waitForGpu() const;

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
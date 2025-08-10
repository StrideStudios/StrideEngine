#pragma once

#include <functional>
#include <memory>
#include <mutex>

#include "ResourceManager.h"

namespace tracy {
	class VkCtx;
}

class CEngineTextures;
class CEngineBuffers;
class CVulkanDevice;

struct SUploadContext {
	std::mutex mMutex;
	VkFence _uploadFence;
	VkCommandPool _commandPool;
	VkCommandBuffer _commandBuffer;
};

class CVulkanRenderer {

public:

	struct FrameData {
		VkCommandPool mCommandPool = nullptr;
		VkCommandBuffer mMainCommandBuffer = nullptr;

		/*
		 * A resource allocator that is persistent for a single frame
		 * Good for data that only needs to exist for a single frame
		 */
		CResourceManager mFrameResourceManager;

		tracy::VkCtx* mTracyContext;
	};

	CVulkanRenderer();

	static void immediateSubmit(std::function<void(VkCommandBuffer cmd)>&& function);

	virtual void init();

	virtual void destroy();

	force_inline FrameData& getCurrentFrame() { return mFrames[getFrameIndex()]; }

	force_inline uint32 getFrameNumber() const { return mFrameNumber; }

	force_inline uint32 getFrameIndex() const { return mFrameNumber % gFrameOverlap; }

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
	CResourceManager mGlobalResourceManager;

	// Stores textures used internally by the engine
	CEngineTextures* mEngineTextures = nullptr;

	// Stores buffers used internally by the engine
	CEngineBuffers* mEngineBuffers = nullptr;

	class CGPUScene* mGPUScene;

	//
	// Rendering Utils
	//

	bool mVSync;

	uint64 mFrameNumber = 0;

	FrameData mFrames[gFrameOverlap];

	SUploadContext mUploadContext;

};

class CNullRenderer final : public CVulkanRenderer {

public:

	void render(VkCommandBuffer cmd) override;
};
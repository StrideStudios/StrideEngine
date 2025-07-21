#pragma once
#include <iostream>
#include <memory>
#include <vulkan/vulkan_core.h>

#include "Common.h"
#include "DeletionQueue.h"
#include "VulkanUtils.h"
#include "Swapchain.h"

class CVulkanDevice;

class CVulkanRenderer {

public:

	struct SFrameData {
		VkCommandPool mCommandPool = nullptr;
		VkCommandBuffer mMainCommandBuffer = nullptr;

		SDeletionQueue mDeletionQueue;
	};

	CVulkanRenderer();

	void destroy();

	force_inline uint32 getFrameNumber() const { return m_FrameNumber; }

	force_inline uint32 getFrameIndex() const { return m_FrameNumber % gFrameOverlap; }

	force_inline const SFrameData& getCurrentFrame() const { return m_Frames[getFrameIndex()]; }

	no_discard const CSwapchain& getSwapchain() const { return *m_Swapchain; }

	// Render to the screen
	void render();

	void drawBackground(VkCommandBuffer cmd) const;

	void waitForGpu();

private:

	//
	// Private non-const getters
	//

	force_inline SFrameData& getCurrentFrame() { return m_Frames[getFrameIndex()]; }

	no_discard CSwapchain& getSwapchain() { return *m_Swapchain; }

	//
	// VMA
	//

	VmaAllocator m_Allocator;

	//
	// Rendering Utils
	//

	uint64 m_FrameNumber = 0;

	SFrameData m_Frames[gFrameOverlap];

	SDeletionQueue m_DeletionQueue;

	SDeletionQueue m_ImageDeletionQueue;

	VkQueue m_GraphicsQueue;
	uint32 m_GraphicsQueueFamily;

	// The swapchain used to present to the screen
	std::unique_ptr<CSwapchain> m_Swapchain = nullptr;

	//
	// Draw resources
	//

	SAllocatedImage m_DrawImage;

};

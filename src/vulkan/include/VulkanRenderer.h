#pragma once
#include <vector>
#include <vulkan/vulkan_core.h>

#include "Common.h"
#include "DeletionQueue.h"
#include "VulkanUtils.h"

struct FrameData {
	VkCommandPool mCommandPool = nullptr;
	VkCommandBuffer mMainCommandBuffer = nullptr;

	VkSemaphore mSwapchainSemaphore = nullptr;
	VkSemaphore mRenderSemaphore = nullptr;
	VkFence mRenderFence = nullptr;
	VkFence mPresentFence = nullptr;

	SDeletionQueue mDeletionQueue;
};

constexpr static uint8 gFrameOverlap = 2;

class CVulkanRenderer {

public:

	explicit CVulkanRenderer(const class CVulkanDevice& inVulkanDevice);

	force_inline FrameData& getCurrentFrame() { return m_Frames[m_FrameNumber % gFrameOverlap]; };

	no_discard int32 getFrameRate() const { return m_FrameRate; }

	no_discard double getDeltaTime() const { return m_DeltaTime; }

	no_discard VkSwapchainKHR getSwapchain() const { return m_VkSwapchain; }

	no_discard VkFormat getFormat() const { return m_VkFormat; }

	// Draw to the screen
	void draw();

	void drawBackground(VkCommandBuffer cmd) const;

	// Stops the GPU and destroys the command Pools
	void cleanup();

private:

	//
	// Frame Data
	//

	uint64 m_FrameNumber = 0;

	int32 m_FrameRate = 0;

	double m_PreviousTime = 0.0;

	double m_DeltaTime = 0.0;

	//
	// VMA
	//

	VmaAllocator m_Allocator;

	//
	// Rendering Utils
	//

	const CVulkanDevice& m_VulkanDevice;

	SDeletionQueue m_DeletionQueue;

	FrameData m_Frames[gFrameOverlap];

	VkQueue m_GraphicsQueue;
	uint32 m_GraphicsQueueFamily;

	//TODO: comments
	VkSwapchainKHR m_VkSwapchain;

	// TODO: Needed if Maintenence1 is not available, should union
	// TODO: this means i probably need some sort of platform info or something
	//std::vector<VkSemaphore> m_RenderSemaphores;

	// The Image format the swapchain is currently using
	VkFormat m_VkFormat;

	std::vector<VkImage> m_VkSwapchainImages;

	std::vector<VkImageView> m_VkSwapchainImageViews;

	VkExtent2D m_VkSwapchainExtent;

	//
	// Draw resources
	//

	SAllocatedImage m_DrawImage;
	VkExtent2D m_DrawExtent;

};

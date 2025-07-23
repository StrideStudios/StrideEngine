#pragma once

#include "ResourceDeallocator.h"
#include "Swapchain.h"
#include "vk_mem_alloc.h"

struct SAllocatedImage {
	VkImage mImage;
	VkImageView mImageView;
	VmaAllocation mAllocation;
	VkExtent3D mImageExtent;
	VkFormat mImageFormat;
};

// Class used to house textures for the engine, easily resizable when necessary
class CEngineTextures {

	friend class CVulkanRenderer;

public:

	CEngineTextures();

	void initializeTextures();

	void reallocate();

	void destroy();

	CSwapchain& getSwapchain() { return m_Swapchain; }

	//
	// Textures
	//

	SAllocatedImage mDrawImage;

private:

	CResourceDeallocator m_ResourceDeallocator;

	//
	// Allocator
	//

	VmaAllocator m_Allocator;

	//
	// SwapChain
	//

	CSwapchain m_Swapchain;

};
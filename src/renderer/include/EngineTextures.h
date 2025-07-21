#pragma once

#include "Swapchain.h"

struct SAllocatedImage {
	VkImage mImage;
	VkImageView mImageView;
	VmaAllocation mAllocation;
	VkExtent3D mImageExtent;
	VkFormat mImageFormat;
};

// Class used to house textures for the engine, easily resizable when necessary
class CEngineTextures {

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

	SDeletionQueue m_DeletionQueue;

	//
	// Allocator
	//

	VmaAllocator m_Allocator;

	//
	// SwapChain
	//

	CSwapchain m_Swapchain;

};
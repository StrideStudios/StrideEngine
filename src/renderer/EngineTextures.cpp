#include "include/EngineTextures.h"

#include "Engine.h"
#include "VulkanDevice.h"

CEngineTextures::CEngineTextures() {

	// initialize the memory allocator
	{
		VmaAllocatorCreateInfo allocatorInfo = {};
		allocatorInfo.physicalDevice = CEngine::get().getDevice().getPhysicalDevice();
		allocatorInfo.device = CEngine::get().getDevice().getDevice();
		allocatorInfo.instance = CEngine::get().getDevice().getInstance();
		allocatorInfo.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
		vmaCreateAllocator(&allocatorInfo, &m_Allocator);
	}

	initializeTextures();
}

void CEngineTextures::initializeTextures() {

	// Ensure previous textures have been destroyed
	m_DeletionQueue.flush();

	//hardcoding the draw format to 32 bit float
	mDrawImage.mImageFormat = VK_FORMAT_R16G16B16A16_SFLOAT;

	VkImageUsageFlags drawImageUsages{};
	drawImageUsages |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
	drawImageUsages |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	drawImageUsages |= VK_IMAGE_USAGE_STORAGE_BIT;
	drawImageUsages |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	auto [width, height] = CEngine::get().getWindow().mExtent;

	mDrawImage.mImageExtent = {width, height, 1};
	VkImageCreateInfo imageInfo = CVulkanInfo::CreateImageInfo(mDrawImage.mImageFormat, drawImageUsages, VkExtent3D(width, height, 1));

	//for the draw image, we want to allocate it from gpu local memory
	VmaAllocationCreateInfo imageAllocationInfo = {};
	imageAllocationInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
	imageAllocationInfo.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	//allocate and create the image
	vmaCreateImage(m_Allocator, &imageInfo, &imageAllocationInfo, &mDrawImage.mImage, &mDrawImage.mAllocation, nullptr);

	//build a image-view for the draw image to use for rendering
	VkImageViewCreateInfo imageViewInfo = CVulkanInfo::CreateImageViewInfo(mDrawImage.mImageFormat, mDrawImage.mImage, VK_IMAGE_ASPECT_COLOR_BIT);

	VK_CHECK(vkCreateImageView(CEngine::get().getDevice().getDevice(), &imageViewInfo, nullptr, &mDrawImage.mImageView));

	//add to deletion queues
	m_DeletionQueue.push([this] {
		vkDestroyImageView(CEngine::get().getDevice().getDevice(), mDrawImage.mImageView, nullptr);
		vmaDestroyImage(m_Allocator, mDrawImage.mImage, mDrawImage.mAllocation);
	});
}


void CEngineTextures::reallocate() {

	auto [x, y] = CEngine::get().getWindow().mExtent;
	msg("Reallocating Engine Textures to ({}, {})", x, y);

	m_Swapchain.recreate();

	initializeTextures();
}

void CEngineTextures::destroy() {

	m_DeletionQueue.flush();

	vmaDestroyAllocator(m_Allocator);

	m_Swapchain.cleanup();
}


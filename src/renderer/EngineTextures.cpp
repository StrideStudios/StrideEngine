#include "EngineTextures.h"

#include "Engine.h"
#include "VulkanDevice.h"

CEngineTextures::CEngineTextures() {

	// initialize the memory allocator
	{
		VmaAllocatorCreateInfo allocatorInfo {
			.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT,
			.physicalDevice = CEngine::get().getDevice().getPhysicalDevice(),
			.device = CEngine::get().getDevice().getDevice(),
			.instance = CEngine::get().getDevice().getInstance()
		};

		vmaCreateAllocator(&allocatorInfo, &m_Allocator);
	}

	initializeTextures();
}

void CEngineTextures::initializeTextures() {

	// Ensure previous textures have been destroyed
	m_ResourceDeallocator.flush();

	//hardcoding the draw format to 32 bit float
	mDrawImage.mImageFormat = VK_FORMAT_R16G16B16A16_SFLOAT;

	VkImageUsageFlags drawImageUsages = 0;
	drawImageUsages |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
	drawImageUsages |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	drawImageUsages |= VK_IMAGE_USAGE_STORAGE_BIT;
	drawImageUsages |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	auto [width, height] = CEngine::get().getWindow().mExtent;

	mDrawImage.mImageExtent = {width, height, 1};
	VkImageCreateInfo imageInfo = CVulkanInfo::CreateImageInfo(mDrawImage.mImageFormat, drawImageUsages, VkExtent3D(width, height, 1));

	//for the draw image, we want to allocate it from gpu local memory
	VmaAllocationCreateInfo imageAllocationInfo = {
		.usage = VMA_MEMORY_USAGE_GPU_ONLY,
		.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT),
	};

	//allocate and create the image
	vmaCreateImage(m_Allocator, &imageInfo, &imageAllocationInfo, &mDrawImage.mImage, &mDrawImage.mAllocation, nullptr);

	//build a image-view for the draw image to use for rendering
	VkImageViewCreateInfo imageViewInfo = CVulkanInfo::CreateImageViewInfo(mDrawImage.mImageFormat, mDrawImage.mImage, VK_IMAGE_ASPECT_COLOR_BIT);

	VK_CHECK(vkCreateImageView(CEngine::get().getDevice().getDevice(), &imageViewInfo, nullptr, &mDrawImage.mImageView));

	//add to resource deallocator
	m_ResourceDeallocator.append({
		&mDrawImage.mImageView,
		{m_Allocator, &mDrawImage.mImage, mDrawImage.mAllocation}
	});
}

void CEngineTextures::reallocate() {

	auto [x, y] = CEngine::get().getWindow().mExtent;
	msg("Reallocating Engine Textures to ({}, {})", x, y);

	m_Swapchain.recreate();

	initializeTextures();
}

void CEngineTextures::destroy() {

	m_ResourceDeallocator.flush();

	vmaDestroyAllocator(m_Allocator);

	m_Swapchain.cleanup();
}


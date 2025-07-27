#include "ResourceManager.h"

#define VMA_IMPLEMENTATION
#include <vma/vk_mem_alloc.h>

#include <vulkan/vk_enum_string_helper.h>

#include "Engine.h"
#include "GraphicsRenderer.h"
#include "VulkanDevice.h"
#include "VulkanRenderer.h"
#include "VulkanUtils.h"
#include "tracy/Tracy.hpp"

void* SBuffer_T::GetMappedData() const {
	return allocation->GetMappedData();
}

#define CREATE_SWITCH(inType, inName, inEnum) \
	case Type::inEnum: \
		deallocate##inName(static_cast<inType>(ptr)); \
		break;

void CResourceManager::Resource::destroy() const {
	switch (mType) {
		FOR_EACH_TYPE(CREATE_SWITCH)
		CREATE_SWITCH(VkPipeline, Pipeline, PIPELINE)
		case Type::BUFFER:
			deallocateBuffer(static_cast<const SBuffer_T *>(mResource.get()));
			break;
		case Type::IMAGE:
			deallocateImage(static_cast<const SImage_T *>(mResource.get()));
			break;
		default:
			astsNoEntry();
	}
}

#undef CREATE_SWITCH

// TODO: turn this into a local that can be used at any point, forcing its own deallocation

void CResourceManager::initAllocator() {
	// initialize the memory allocator
	VmaAllocatorCreateInfo allocatorInfo {
		.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT,
		.physicalDevice = CEngine::physicalDevice(),
		.device = CEngine::device(),
		.instance = CEngine::instance()
	};

	VK_CHECK(vmaCreateAllocator(&allocatorInfo, &getAllocator()));
}

void CResourceManager::destroyAllocator() {
	vmaDestroyAllocator(getAllocator());
}

#define DEFINE_ALLOCATER(inType, inName, inEnum) \
	inType CResourceManager::allocate##inName(const inType##CreateInfo& pCreateInfo, const VkAllocationCallbacks* pAllocator) { \
		ZoneScopedN("Allocate " #inName); \
		inType inName; \
		VK_CHECK(vkCreate##inName(CEngine::device(), &pCreateInfo, pAllocator, &inName)); \
		push(inName); \
		return inName; \
	}
#define DEFINE_DEALLOCATOR(inType, inName, inEnum) \
	void CResourceManager::deallocate##inName(const inType& inName) { \
		vkDestroy##inName(CEngine::device(), inName, nullptr); \
	}

	FOR_EACH_TYPE(DEFINE_ALLOCATER)
	FOR_EACH_TYPE(DEFINE_DEALLOCATOR)

	// Pipeline has a non-standard allocater
	DEFINE_DEALLOCATOR(VkPipeline, Pipeline, PIPELINE);

#undef DEFINE_FUNCTIONS
#undef DEFINE_DEALLOCATOR

VkCommandBuffer CResourceManager::allocateCommandBuffer(const VkCommandBufferAllocateInfo& pCreateInfo) {
	ZoneScopedN("Allocate CommandBuffer");
	VkCommandBuffer Buffer;
	VK_CHECK(vkAllocateCommandBuffers(CEngine::device(), &pCreateInfo, &Buffer));
	return Buffer;
}

VkPipeline CResourceManager::allocatePipeline(const CPipelineBuilder& inPipelineBuilder, const VkAllocationCallbacks* pAllocator) {
	ZoneScopedN("Allocate Pipeline");
	const VkPipeline Pipeline = inPipelineBuilder.buildPipeline(CEngine::device());
	push(Pipeline);
	return Pipeline;
}

SImage CResourceManager::allocateImage(VkExtent3D inExtent, VkFormat inFormat, VkImageUsageFlags inFlags, VkImageAspectFlags inViewFlags, bool inMipmapped) {
	ZoneScopedN("Allocate Image");
	auto image = std::make_shared<SImage_T>();

	image->mImageExtent = inExtent;
	image->mImageFormat = inFormat;

	VkImageCreateInfo imageInfo = CVulkanInfo::createImageInfo(image->mImageFormat, inFlags, image->mImageExtent);
	if (inMipmapped) {
		imageInfo.mipLevels = static_cast<uint32>(std::floor(std::log2(std::max(inExtent.width, inExtent.height)))) + 1;
	}

	//for the draw image, we want to allocate it from gpu local memory
	VmaAllocationCreateInfo imageAllocationInfo = {
		.usage = VMA_MEMORY_USAGE_GPU_ONLY,
		.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT),
	};

	vmaCreateImage(getAllocator(), &imageInfo, &imageAllocationInfo, &image->mImage, &image->mAllocation, nullptr);

	//build a image-view for the draw image to use for rendering
	VkImageViewCreateInfo imageViewInfo = CVulkanInfo::createImageViewInfo(image->mImageFormat, image->mImage, inViewFlags);
	imageViewInfo.subresourceRange.levelCount = imageInfo.mipLevels;

	VK_CHECK(vkCreateImageView(CEngine::device(), &imageViewInfo, nullptr, &image->mImageView));

	push(image);

	return image;
}

void generateMipmaps(VkCommandBuffer cmd, VkImage image, VkExtent2D extent) {
	int mipLevels = int(std::floor(std::log2(std::max(extent.width, extent.height)))) + 1;
    for (int mip = 0; mip < mipLevels; mip++) {

        VkExtent2D halfSize = extent;
        halfSize.width /= 2;
        halfSize.height /= 2;

        VkImageMemoryBarrier2 imageBarrier { .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2, .pNext = nullptr };

        imageBarrier.srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
        imageBarrier.srcAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT;
        imageBarrier.dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
        imageBarrier.dstAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT | VK_ACCESS_2_MEMORY_READ_BIT;

        imageBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        imageBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;

        VkImageAspectFlags aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        imageBarrier.subresourceRange = CVulkanUtils::imageSubresourceRange(aspectMask);
        imageBarrier.subresourceRange.levelCount = 1;
        imageBarrier.subresourceRange.baseMipLevel = mip;
        imageBarrier.image = image;

        VkDependencyInfo depInfo { .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO, .pNext = nullptr };
        depInfo.imageMemoryBarrierCount = 1;
        depInfo.pImageMemoryBarriers = &imageBarrier;

        vkCmdPipelineBarrier2(cmd, &depInfo);

        if (mip < mipLevels - 1) {
            VkImageBlit2 blitRegion { .sType = VK_STRUCTURE_TYPE_IMAGE_BLIT_2, .pNext = nullptr };

            blitRegion.srcOffsets[1].x = extent.width;
            blitRegion.srcOffsets[1].y = extent.height;
            blitRegion.srcOffsets[1].z = 1;

            blitRegion.dstOffsets[1].x = halfSize.width;
            blitRegion.dstOffsets[1].y = halfSize.height;
            blitRegion.dstOffsets[1].z = 1;

            blitRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            blitRegion.srcSubresource.baseArrayLayer = 0;
            blitRegion.srcSubresource.layerCount = 1;
            blitRegion.srcSubresource.mipLevel = mip;

            blitRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            blitRegion.dstSubresource.baseArrayLayer = 0;
            blitRegion.dstSubresource.layerCount = 1;
            blitRegion.dstSubresource.mipLevel = mip + 1;

            VkBlitImageInfo2 blitInfo {.sType = VK_STRUCTURE_TYPE_BLIT_IMAGE_INFO_2, .pNext = nullptr};
            blitInfo.dstImage = image;
            blitInfo.dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            blitInfo.srcImage = image;
            blitInfo.srcImageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            blitInfo.filter = VK_FILTER_LINEAR;
            blitInfo.regionCount = 1;
            blitInfo.pRegions = &blitRegion;

            vkCmdBlitImage2(cmd, &blitInfo);

            extent = halfSize;
        }
    }

    // transition all mip levels into the final read_only layout
    CVulkanUtils::transitionImage(cmd, image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
}

SImage CResourceManager::allocateImage(void* inData, VkExtent3D inExtent, VkFormat inFormat, VkImageUsageFlags inFlags, VkImageAspectFlags inViewFlags, bool inMipmapped) {
	size_t data_size = inExtent.depth * inExtent.width * inExtent.height * 4;

	// Upload buffer is not needed outside of this function
	CResourceManager manager;
	const SBuffer uploadBuffer = manager.allocateBuffer(data_size, VMA_MEMORY_USAGE_CPU_TO_GPU, VK_BUFFER_USAGE_TRANSFER_SRC_BIT);

	memcpy(uploadBuffer->info.pMappedData, inData, data_size);

	SImage new_image = allocateImage(inExtent, inFormat, inFlags | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, inViewFlags, inMipmapped);

	CVulkanRenderer::immediateSubmit([&](VkCommandBuffer cmd) {
		CVulkanUtils::transitionImage(cmd, new_image->mImage, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

		VkBufferImageCopy copyRegion = {};
		copyRegion.bufferOffset = 0;
		copyRegion.bufferRowLength = 0;
		copyRegion.bufferImageHeight = 0;

		copyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		copyRegion.imageSubresource.mipLevel = 0;
		copyRegion.imageSubresource.baseArrayLayer = 0;
		copyRegion.imageSubresource.layerCount = 1;
		copyRegion.imageExtent = inExtent;

		// copy the buffer into the image
		vkCmdCopyBufferToImage(cmd, uploadBuffer->buffer, new_image->mImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1,
			&copyRegion);

		if (inMipmapped) {
			generateMipmaps(cmd, new_image->mImage, VkExtent2D{new_image->mImageExtent.width,new_image->mImageExtent.height});
		} else {
			CVulkanUtils::transitionImage(cmd, new_image->mImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
			}
		}
	);

	manager.flush();

	return new_image;
}

void CResourceManager::deallocateImage(const SImage_T* inImage) {
	vmaDestroyImage(getAllocator(), inImage->mImage, inImage->mAllocation);
	vkDestroyImageView(CEngine::device(), inImage->mImageView, nullptr);
}

SBuffer CResourceManager::allocateBuffer(size_t allocSize, VmaMemoryUsage memoryUsage, VkBufferUsageFlags usage) {
	ZoneScopedN("Allocate Buffer");
	// allocate buffer
	VkBufferCreateInfo bufferInfo = {
		.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		.pNext = nullptr,
		.size = allocSize,
		.usage = usage
	};

	VmaAllocationCreateInfo vmaallocInfo = {
		.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT,
		.usage = memoryUsage
	};

	auto buffer = std::make_shared<SBuffer_T>();

	// allocate the buffer
	VK_CHECK(vmaCreateBuffer(getAllocator(), &bufferInfo, &vmaallocInfo, &buffer->buffer, &buffer->allocation, &buffer->info));

	push(buffer);

	return buffer;
}

SMeshBuffers CResourceManager::allocateMeshBuffer(size_t indicesSize, size_t verticesSize) {
	const VkDevice device = CEngine::device();

	// Although it would normally be bad practice to return a heap allocated pointer
	// CAllocator (so long as inShouldDeallocate is true) will automatically destroy the pointer
	// if not, then the user can call CAllocator::deallocate
	auto meshBuffers = std::make_shared<SMeshBuffers_T>();

	meshBuffers->indexBuffer = allocateBuffer(indicesSize, VMA_MEMORY_USAGE_GPU_ONLY, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);

	meshBuffers->vertexBuffer = allocateBuffer(verticesSize, VMA_MEMORY_USAGE_GPU_ONLY, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT);

	//find the address of the vertex buffer
	// TODO: pointer math for large buffers
	VkBufferDeviceAddressInfo deviceAddressInfo {
		.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
		.buffer = meshBuffers->vertexBuffer->buffer
	};

	meshBuffers->vertexBufferAddress = vkGetBufferDeviceAddress(device, &deviceAddressInfo);

	return meshBuffers;
}

void CResourceManager::deallocateBuffer(const SBuffer_T* inBuffer) {
	vmaDestroyBuffer(getAllocator(), inBuffer->buffer, inBuffer->allocation);
}
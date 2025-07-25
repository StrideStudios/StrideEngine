#include "ResourceManager.h"

#define VMA_IMPLEMENTATION
#include <vma/vk_mem_alloc.h>

#include <vulkan/vk_enum_string_helper.h>

#include "Engine.h"
#include "GraphicsRenderer.h"
#include "VulkanDevice.h"
#include "VulkanRenderer.h"
#include "VulkanUtils.h"

void* SBuffer_T::GetMappedData() const {
	return allocation->GetMappedData();
}

#define CREATE_SWITCH(inType, inName, inEnum) \
	case Type::inEnum: \
		deallocate##inName(mResource.inName); \
		break;

void CResourceManager::Resource::destroy() const {
	switch (mType) {
		FOR_EACH_TYPE(CREATE_SWITCH)
		case Type::BUFFER:
			deallocateBuffer(mResource.Buffer);
			break;
		case Type::IMAGE:
			deallocateImage(mResource.Image);
			break;
		case Type::PIPELINE:
			deallocatePipeline(mResource.Pipeline);
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
	VkCommandBuffer Buffer;
	VK_CHECK(vkAllocateCommandBuffers(CEngine::device(), &pCreateInfo, &Buffer));
	return Buffer;
}

VkPipeline CResourceManager::allocatePipeline(const CPipelineBuilder& inPipelineBuilder, const VkAllocationCallbacks* pAllocator) {
	const VkPipeline Pipeline = inPipelineBuilder.buildPipeline(CEngine::device());
	push(Pipeline);
	return Pipeline;
}

SImage CResourceManager::allocateImage(VkExtent3D inExtent, VkFormat inFormat, VkImageUsageFlags inFlags, VkImageAspectFlags inViewFlags, bool inMipmapped) {
	auto image = std::make_unique<SImage_T>();

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

		CVulkanUtils::transitionImage(cmd, new_image->mImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		});

	manager.flush();

	return new_image;
}

void CResourceManager::deallocateImage(const SImage_T* inImage) {
	vmaDestroyImage(getAllocator(), inImage->mImage, inImage->mAllocation);
	vkDestroyImageView(CEngine::device(), inImage->mImageView, nullptr);
}

SBuffer CResourceManager::allocateBuffer(size_t allocSize, VmaMemoryUsage memoryUsage, VkBufferUsageFlags usage) {
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

	auto buffer = std::make_unique<SBuffer_T>();

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
	auto meshBuffers = std::make_unique<SMeshBuffers_T>();

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
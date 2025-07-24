#include "ResourceAllocator.h"

#define VMA_IMPLEMENTATION
#include <vma/vk_mem_alloc.h>

#include <vulkan/vk_enum_string_helper.h>

#include "Engine.h"
#include "VulkanDevice.h"
#include "VulkanUtils.h"

void* SBuffer_T::GetMappedData() const {
	return allocation->GetMappedData();
}

void CResourceAllocator::init() {
	// initialize the memory allocator
	VmaAllocatorCreateInfo allocatorInfo {
		.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT,
		.physicalDevice = CEngine::physicalDevice(),
		.device = CEngine::device(),
		.instance = CEngine::instance()
	};

	VK_CHECK(vmaCreateAllocator(&allocatorInfo, &get().m_Allocator));
}

SImage CResourceAllocator::allocateImage(VkExtent3D inExtent, VkFormat inFormat, VkImageUsageFlags inFlags, VkImageAspectFlags inViewFlags, bool inShouldDeallocate) {
	auto image = std::make_unique<SImage_T>();

	image->mImageExtent = inExtent;
	image->mImageFormat = inFormat;

	VkImageCreateInfo imageInfo = CVulkanInfo::createImageInfo(image->mImageFormat, inFlags, image->mImageExtent);

	//for the draw image, we want to allocate it from gpu local memory
	VmaAllocationCreateInfo imageAllocationInfo = {
		.usage = VMA_MEMORY_USAGE_GPU_ONLY,
		.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT),
	};

	vmaCreateImage(get().m_Allocator, &imageInfo, &imageAllocationInfo, &image->mImage, &image->mAllocation, nullptr);

	//build a image-view for the draw image to use for rendering
	VkImageViewCreateInfo imageViewInfo = CVulkanInfo::createImageViewInfo(image->mImageFormat, image->mImage, inViewFlags);

	VK_CHECK(vkCreateImageView(CEngine::device(), &imageViewInfo, nullptr, &image->mImageView));

	// Textures for the screen have their own deallocators
	if (inShouldDeallocate) get().m_ResourceDeallocator.push(image);

	return image;
}

void CResourceAllocator::deallocateImage(const SImage_T* inImage) {
	vmaDestroyImage(get().m_Allocator, inImage->mImage, inImage->mAllocation);
	vkDestroyImageView(CEngine::device(), inImage->mImageView, nullptr);
}

SBuffer CResourceAllocator::allocateBuffer(size_t allocSize, VmaMemoryUsage memoryUsage, VkBufferUsageFlags usage, bool inShouldDeallocate) {
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

	// Although it would normally be bad practice to return a heap allocated pointer
	// CAllocator (so long as inShouldDeallocate is true) will automatically destroy the pointer
	// if not, then the user can call CAllocator::deallocate
	auto buffer = std::make_unique<SBuffer_T>();

	// allocate the buffer
	VK_CHECK(vmaCreateBuffer(get().m_Allocator, &bufferInfo, &vmaallocInfo, &buffer->buffer, &buffer->allocation, &buffer->info));

	if (inShouldDeallocate) get().m_ResourceDeallocator.push(buffer);

	return buffer;
}

void CResourceAllocator::deallocateBuffer(const SBuffer_T* inBuffer) {
	vmaDestroyBuffer(get().m_Allocator, inBuffer->buffer, inBuffer->allocation);
}

SMeshBuffers CResourceAllocator::allocateMeshBuffer(size_t indicesSize, size_t verticesSize, bool inShouldDeallocate) {
	const VkDevice device = CEngine::device();

	// Although it would normally be bad practice to return a heap allocated pointer
	// CAllocator (so long as inShouldDeallocate is true) will automatically destroy the pointer
	// if not, then the user can call CAllocator::deallocate
	auto meshBuffers = std::make_unique<SMeshBuffers_T>();

	meshBuffers->indexBuffer = allocateBuffer(indicesSize, VMA_MEMORY_USAGE_GPU_ONLY, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, false);

	meshBuffers->vertexBuffer = allocateBuffer(verticesSize, VMA_MEMORY_USAGE_GPU_ONLY, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, false);

	//find the address of the vertex buffer
	// TODO: pointer math for large buffers
	VkBufferDeviceAddressInfo deviceAddressInfo {
		.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
		.buffer = meshBuffers->vertexBuffer->buffer
	};

	meshBuffers->vertexBufferAddress = vkGetBufferDeviceAddress(device, &deviceAddressInfo);

	if (inShouldDeallocate) get().m_ResourceDeallocator.push(meshBuffers);

	return meshBuffers;
}

void CResourceAllocator::deallocateMeshBuffer(const SMeshBuffers_T* inBuffer) {
	deallocateBuffer(inBuffer->indexBuffer);
	deallocateBuffer(inBuffer->vertexBuffer);
}

void CResourceAllocator::destroy() {
	get().m_ResourceDeallocator.flush();
	vmaDestroyAllocator(get().m_Allocator);
}


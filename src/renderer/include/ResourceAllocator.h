#pragma once

#include <memory>
#include <vma/vk_mem_alloc.h>

#include "Common.h"
#include "ResourceDeallocator.h"

class CVulkanDevice;

struct SImage_T {
	VkImage mImage;
	VkImageView mImageView;
	VmaAllocation mAllocation;
	VkExtent3D mImageExtent;
	VkFormat mImageFormat;
};
typedef std::unique_ptr<SImage_T> SImage;

struct SBuffer_T {
	VkBuffer buffer = nullptr;
	VmaAllocation allocation = nullptr;
	VmaAllocationInfo info = {};

	no_discard void* GetMappedData() const;
};
typedef std::unique_ptr<SBuffer_T> SBuffer;

// Holds the resources needed for mesh rendering
struct SMeshBuffers_T {
	SBuffer indexBuffer = nullptr;
	SBuffer vertexBuffer = nullptr;
	VkDeviceAddress vertexBufferAddress;
};
typedef std::unique_ptr<SMeshBuffers_T> SMeshBuffers;

class CResourceAllocator {

	constexpr static VmaAllocator& getAllocator() {
		static VmaAllocator allocator;
		return allocator;
	}

public:

	CResourceAllocator() = default;

	static void initAllocator();

	static void destroyAllocator();

	void flush();

	SImage allocateImage(VkExtent3D inExtent, VkFormat inFormat, VkImageUsageFlags inFlags = 0, VkImageAspectFlags inViewFlags = 0, bool inMipmapped = false);

	SImage allocateImage(void* inData, VkExtent3D inExtent, VkFormat inFormat, VkImageUsageFlags inFlags = 0, VkImageAspectFlags inViewFlags = 0, bool inMipmapped = false);

	static void deallocateImage(const SImage_T* inImage);

	static void deallocateImage(const SImage& inImage) { deallocateImage(inImage.get()); }

	// VkBufferUsageFlags is VERY important (VMA_MEMORY_USAGE_GPU_ONLY, VMA_MEMORY_USAGE_CPU_ONLY, VMA_MEMORY_USAGE_CPU_TO_GPU, VMA_MEMORY_USAGE_GPU_TO_CPU)
	// VMA_MEMORY_USAGE_CPU_TO_GPU can be used for the small fast-access buffer on GPU that CPU can still write to (something important)
	SBuffer allocateBuffer(size_t allocSize, VmaMemoryUsage memoryUsage, VkBufferUsageFlags usage = 0);

	static void deallocateBuffer(const SBuffer_T* inBuffer);

	static void deallocateBuffer(const SBuffer& inImage) { deallocateBuffer(inImage.get()); }

	SMeshBuffers allocateMeshBuffer(size_t indicesSize, size_t verticesSize);

private:

	CResourceDeallocator m_ResourceDeallocator{};
};

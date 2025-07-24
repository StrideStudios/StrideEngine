#pragma once

#include <memory>
#include <vma/vk_mem_alloc.h>
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

	void* GetMappedData() const;
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

public:

	CResourceAllocator() = default;

	constexpr static CResourceAllocator& get() {
		static CResourceAllocator allocator;
		return allocator;
	}

	static void init();

	static void destroy();

	static SImage allocateImage(VkExtent3D inExtent, VkFormat inFormat, VkImageUsageFlags inFlags = 0, VkImageAspectFlags inViewFlags = 0, bool inShouldDeallocate = true);

	static void deallocateImage(const SImage_T* inImage);

	static void deallocateImage(const SImage& inImage) { deallocateImage(inImage.get()); }

	// VkBufferUsageFlags is VERY important (VMA_MEMORY_USAGE_GPU_ONLY, VMA_MEMORY_USAGE_CPU_ONLY, VMA_MEMORY_USAGE_CPU_TO_GPU, VMA_MEMORY_USAGE_GPU_TO_CPU)
	// VMA_MEMORY_USAGE_CPU_TO_GPU can be used for the small fast-access buffer on GPU that CPU can still write to (something important)
	static SBuffer allocateBuffer(size_t allocSize, VmaMemoryUsage memoryUsage, VkBufferUsageFlags usage = 0, bool inShouldDeallocate = true);

	static void deallocateBuffer(const SBuffer_T* inBuffer);

	static void deallocateBuffer(const SBuffer& inImage) { deallocateBuffer(inImage.get()); }

	static SMeshBuffers allocateMeshBuffer(size_t indicesSize, size_t verticesSize, bool inShouldDeallocate = true);

	static void deallocateMeshBuffer(const SMeshBuffers_T* inBuffer);

	static void deallocateMeshBuffer(const SMeshBuffers& inImage) { deallocateMeshBuffer(inImage.get()); }

public:

	//
	// Allocator
	//

	CResourceDeallocator m_ResourceDeallocator;

	VmaAllocator m_Allocator;
};

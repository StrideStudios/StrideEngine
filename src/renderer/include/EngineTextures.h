#pragma once

#include "ResourceDeallocator.h"
#include "Swapchain.h"
#include "vk_mem_alloc.h"

struct SUploadContext {
	VkFence _uploadFence;
	VkCommandPool _commandPool;
	VkCommandBuffer _commandBuffer;
};

struct SVertex {
	Vector3f position;
	float uv_x;
	Vector3f normal;
	float uv_y;
	Vector4f color;
};

struct SAllocatedImage {
	VkImage mImage;
	VkImageView mImageView;
	VmaAllocation mAllocation;
	VkExtent3D mImageExtent;
	VkFormat mImageFormat;
};

struct SAllocatedBuffer {
	VkBuffer buffer = nullptr;
	VmaAllocation allocation = nullptr;
	VmaAllocationInfo info = {};
};

// holds the resources needed for a mesh
struct SGPUMeshBuffers {

	SAllocatedBuffer indexBuffer{};
	SAllocatedBuffer vertexBuffer{};
	VkDeviceAddress vertexBufferAddress;
};

// Class used to house textures for the engine, easily resizable when necessary
class CEngineTextures {

	friend class CVulkanRenderer;

public:

	CEngineTextures(CVulkanRenderer* renderer);

	void initDefaultData(CVulkanRenderer* renderer);

	void initializeTextures();

	void immediateSubmit(CVulkanRenderer* renderer, std::function<void(VkCommandBuffer cmd)>&& function);

	// TODO: SHOULD BE HANDLED BY ENGINE TEXTURES
	// VkBufferUsageFlags is VERY important (VMA_MEMORY_USAGE_GPU_ONLY, VMA_MEMORY_USAGE_CPU_ONLY, VMA_MEMORY_USAGE_CPU_TO_GPU, VMA_MEMORY_USAGE_GPU_TO_CPU)
	// VMA_MEMORY_USAGE_CPU_TO_GPU can be used for the small fast-access buffer on GPU that CPU can still write to (something important)
	SAllocatedBuffer createBuffer(size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage);

	SGPUMeshBuffers uploadMesh(CVulkanRenderer* renderer, std::span<uint32> indices, std::span<SVertex> vertices);

	void destroyBuffer(const SAllocatedBuffer& inBuffer);

	void reallocate();

	void destroy();

	CSwapchain& getSwapchain() { return m_Swapchain; }

	SUploadContext m_UploadContext;

	//
	// Textures
	//

	SAllocatedImage mDrawImage;

	//
	// Buffers
	//

	SGPUMeshBuffers mRectangle; // For Graphics Renderer should do vector but for now this works

	//
	// Allocator
	//

	VmaAllocator mAllocator;

private:

	CResourceDeallocator m_ResourceDeallocator;

	CResourceDeallocator m_BufferResourceDeallocator;

	//
	// SwapChain
	//

	CSwapchain m_Swapchain;

};
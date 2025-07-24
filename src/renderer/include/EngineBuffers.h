#pragma once

#include <span>

#include "ResourceDeallocator.h"
#include "Swapchain.h"
#include "vk_mem_alloc.h"

struct SUploadContext {
	VkFence _uploadFence;
	VkCommandPool _commandPool;
	VkCommandBuffer _commandBuffer;
};

// Represents a vertex
struct SVertex {
	Vector3f position;
	float uv_x;
	Vector3f normal;
	float uv_y;
	Vector4f color;
};

class CEngineBuffers {

	friend class CVulkanRenderer;

public:

	CEngineBuffers(CVulkanRenderer* renderer);

	void destroy();

	void initDefaultData(CVulkanRenderer* renderer);

	void immediateSubmit(CVulkanRenderer* renderer, std::function<void(VkCommandBuffer cmd)>&& function);

	void uploadMesh(CVulkanRenderer* renderer, std::span<uint32> indices, std::span<SVertex> vertices);

	SUploadContext m_UploadContext;

	//
	// Buffers
	//

	SMeshBuffers mRectangle; // For Graphics Renderer should do vector but for now this works

};
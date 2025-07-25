#pragma once

#include <span>

#include "ResourceAllocator.h"
#include "ResourceDeallocator.h"
#include "Swapchain.h"
#include "vk_mem_alloc.h"

struct SStaticMesh;

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

	void initDefaultData(CVulkanRenderer* renderer);

	SMeshBuffers uploadMesh(CResourceAllocator& inAllocator, std::span<uint32> indices, std::span<SVertex> vertices);

	//
	// Buffers
	//

	SMeshBuffers mRectangle; // For Graphics Renderer should do vector but for now this works

	std::vector<std::shared_ptr<SStaticMesh>> testMeshes;

};
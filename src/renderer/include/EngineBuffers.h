#pragma once

#include <span>

#include "ResourceManager.h"
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

class CEngineBuffers : public IDestroyable {

	friend class CVulkanRenderer;

public:

	CEngineBuffers();

	~CEngineBuffers();

	SMeshBuffers uploadMesh(CResourceManager& inManager, std::span<uint32> indices, std::span<SVertex> vertices);

};
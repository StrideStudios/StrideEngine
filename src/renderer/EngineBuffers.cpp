#include "EngineBuffers.h"

#include <array>

#include "Engine.h"
#include "VulkanRenderer.h"
#include "Common.h"
#include "MeshLoader.h"
#include "ResourceManager.h"

CEngineBuffers::CEngineBuffers(CVulkanRenderer* renderer) {
	initDefaultData(renderer);
}

CEngineBuffers::~CEngineBuffers() = default;

void CEngineBuffers::initDefaultData(CVulkanRenderer* renderer) {
	std::array<SVertex,4> rect_vertices;

	rect_vertices[0].position = {0.5,-0.5, 0};
	rect_vertices[1].position = {0.5,0.5, 0};
	rect_vertices[2].position = {-0.5,-0.5, 0};
	rect_vertices[3].position = {-0.5,0.5, 0};

	rect_vertices[0].color = {0,0, 0,1};
	rect_vertices[1].color = { 0.5,0.5,0.5 ,1};
	rect_vertices[2].color = { 1,0, 0,1 };
	rect_vertices[3].color = { 0,1, 0,1 };

	std::array<uint32,6> rect_indices;

	rect_indices[0] = 0;
	rect_indices[1] = 1;
	rect_indices[2] = 2;

	rect_indices[3] = 2;
	rect_indices[4] = 1;
	rect_indices[5] = 3;

	mRectangle = uploadMesh(renderer->mGlobalResourceManager, rect_indices, rect_vertices);

	testMeshes = CMeshLoader::loadStaticMesh(renderer, this, "basicmesh.glb").value();
}

SMeshBuffers CEngineBuffers::uploadMesh(CResourceManager& inManager, std::span<uint32> indices, std::span<SVertex> vertices) {
	const size_t vertexBufferSize = vertices.size() * sizeof(SVertex);
	const size_t indexBufferSize = indices.size() * sizeof(uint32);

	// Create buffers
	SMeshBuffers meshBuffers = inManager.allocateMeshBuffer(indexBufferSize, vertexBufferSize);

	// Staging is not needed outside of this function
	CResourceManager manager;
	const SBuffer staging = manager.allocateBuffer(vertexBufferSize + indexBufferSize, VMA_MEMORY_USAGE_CPU_ONLY, VK_BUFFER_USAGE_TRANSFER_SRC_BIT);

	void* data = staging->GetMappedData();

	// copy vertex buffer
	memcpy(data, vertices.data(), vertexBufferSize);
	// copy index buffer
	memcpy((char*)data + vertexBufferSize, indices.data(), indexBufferSize);

	//TODO: slow, render thread?
	// also from an older version of the tutorial, doesnt use sync2
	CVulkanRenderer::immediateSubmit([&](VkCommandBuffer cmd) {
		VkBufferCopy vertexCopy{};
		vertexCopy.dstOffset = 0;
		vertexCopy.srcOffset = 0;
		vertexCopy.size = vertexBufferSize;

		vkCmdCopyBuffer(cmd, staging->buffer, meshBuffers->vertexBuffer->buffer, 1, &vertexCopy);

		VkBufferCopy indexCopy{};
		indexCopy.dstOffset = 0;
		indexCopy.srcOffset = vertexBufferSize;
		indexCopy.size = indexBufferSize;

		vkCmdCopyBuffer(cmd, staging->buffer, meshBuffers->indexBuffer->buffer, 1, &indexCopy);
	});

	manager.flush();

	return meshBuffers;
}
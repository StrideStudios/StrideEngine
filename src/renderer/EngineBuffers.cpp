#include "EngineBuffers.h"

#include "Engine.h"
#include "VulkanRenderer.h"
#include "Common.h"
#include "ResourceManager.h"

CEngineBuffers::CEngineBuffers() = default;

CEngineBuffers::~CEngineBuffers() = default;

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
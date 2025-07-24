#include "EngineBuffers.h"

#include <array>

#include "Engine.h"
#include "Common.h"
#include "ResourceAllocator.h"

CEngineBuffers::CEngineBuffers(CVulkanRenderer* renderer) {

	VkCommandPoolCreateInfo uploadCommandPoolInfo = CVulkanInfo::createCommandPoolInfo(renderer->m_GraphicsQueueFamily);
	//create pool for upload context
	VK_CHECK(vkCreateCommandPool(CEngine::device(), &uploadCommandPoolInfo, nullptr, &m_UploadContext._commandPool));

	//allocate the default command buffer that we will use for the instant commands
	VkCommandBufferAllocateInfo cmdAllocInfo = CVulkanInfo::createCommandAllocateInfo(m_UploadContext._commandPool, 1);
	VK_CHECK(vkAllocateCommandBuffers(CEngine::device(), &cmdAllocInfo, &m_UploadContext._commandBuffer));

	VkFenceCreateInfo fenceCreateInfo = CVulkanInfo::createFenceInfo(VK_FENCE_CREATE_SIGNALED_BIT);
	VK_CHECK(vkCreateFence(CEngine::device(), &fenceCreateInfo, nullptr, &m_UploadContext._uploadFence));

	initDefaultData(renderer);
}

void CEngineBuffers::destroy() {
	vkDestroyCommandPool(CEngine::device(), m_UploadContext._commandPool, nullptr);
	vkDestroyFence(CEngine::device(), m_UploadContext._uploadFence, nullptr);
}

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

	uploadMesh(renderer, rect_indices, rect_vertices);
}

void CEngineBuffers::immediateSubmit(CVulkanRenderer* renderer, std::function<void(VkCommandBuffer cmd)>&& function) {
	VkDevice device = CEngine::device();

	VkCommandBuffer cmd = m_UploadContext._commandBuffer;

	//begin the command buffer recording. We will use this command buffer exactly once before resetting, so we tell vulkan that
	VkCommandBufferBeginInfo cmdBeginInfo = CVulkanInfo::createCommandBufferBeginInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

	VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));

	//execute the function
	function(cmd);

	VK_CHECK(vkEndCommandBuffer(cmd));

	VkSubmitInfo submit = CVulkanInfo::submitInfo(&cmd);

	vkResetFences(device, 1, &m_UploadContext._uploadFence);

	//submit command buffer to the queue and execute it.
	// _uploadFence will now block until the graphic commands finish execution
	VK_CHECK(vkQueueSubmit(renderer->m_GraphicsQueue, 1, &submit, m_UploadContext._uploadFence));

	vkWaitForFences(device, 1, &m_UploadContext._uploadFence, true, 9999999999);

	// reset the command buffers inside the command pool
	vkResetCommandPool(device, m_UploadContext._commandPool, 0);
}

void CEngineBuffers::uploadMesh(CVulkanRenderer* renderer, std::span<uint32> indices, std::span<SVertex> vertices) {
	const size_t vertexBufferSize = vertices.size() * sizeof(SVertex);
	const size_t indexBufferSize = indices.size() * sizeof(uint32);

	// Create buffers
	mRectangle = CResourceAllocator::allocateMeshBuffer(indexBufferSize, vertexBufferSize);

	const SBuffer staging = CResourceAllocator::allocateBuffer(vertexBufferSize + indexBufferSize, VMA_MEMORY_USAGE_CPU_ONLY, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, false);

	void* data = staging->GetMappedData();

	// copy vertex buffer
	memcpy(data, vertices.data(), vertexBufferSize);
	// copy index buffer
	memcpy((char*)data + vertexBufferSize, indices.data(), indexBufferSize);

	//TODO: slow, render thread?
	// also from an older version of the tutorial, doesnt use sync2
	immediateSubmit(renderer, [&](VkCommandBuffer cmd) {
		VkBufferCopy vertexCopy{};
		vertexCopy.dstOffset = 0;
		vertexCopy.srcOffset = 0;
		vertexCopy.size = vertexBufferSize;

		vkCmdCopyBuffer(cmd, staging->buffer, mRectangle->vertexBuffer->buffer, 1, &vertexCopy);

		VkBufferCopy indexCopy{};
		indexCopy.dstOffset = 0;
		indexCopy.srcOffset = vertexBufferSize;
		indexCopy.size = indexBufferSize;

		vkCmdCopyBuffer(cmd, staging->buffer, mRectangle->indexBuffer->buffer, 1, &indexCopy);
	});

	CResourceAllocator::deallocateBuffer(staging);
}
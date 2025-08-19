#include "passes/Pass.h"

#include "renderer/Material.h"
#include "Viewport.h"
#include "renderer/VulkanRenderer.h"
#include "renderer/VulkanUtils.h"
#include "renderer/VulkanResourceManager.h"

void SInstancer::reallocate(const Matrix4f& parentMatrix)  {

	std::vector<SInstance> inputData = instances;

	for (auto& instance : inputData) {
		instance.Transform = parentMatrix * instance.Transform;
	}

	instanceManager.flush();

	const size_t bufferSize = inputData.size() * sizeof(SInstance);

	instanceBuffer = instanceManager.allocateBuffer(bufferSize, VMA_MEMORY_USAGE_GPU_ONLY, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);

	// Staging is not needed outside of this function
	CVulkanResourceManager manager;
	const SBuffer_T* staging = manager.allocateBuffer(bufferSize, VMA_MEMORY_USAGE_CPU_ONLY, VK_BUFFER_USAGE_TRANSFER_SRC_BIT);

	void* data = staging->GetMappedData();
	memcpy(data, inputData.data(), bufferSize);

	// TODO: shouldnt have instancer here, but its necessary for instanced static mesh object...
	CVulkanRenderer::get()->immediateSubmit([&](VkCommandBuffer cmd) {
		VkBufferCopy vertexCopy{};
		vertexCopy.dstOffset = 0;
		vertexCopy.srcOffset = 0;
		vertexCopy.size = bufferSize;

		vkCmdCopyBuffer(cmd, staging->buffer, instanceBuffer->buffer, 1, &vertexCopy);
	});

	manager.flush();
}

void CPass::bindPipeline(const VkCommandBuffer cmd, CPipeline* inPipeline, const SPushConstants& inConstants) {
	//if (surface.material != lastMaterial) {
	//lastMaterial = surface.material;
	// If the pipeline has changed, rebind pipeline data
	if (inPipeline != m_CurrentPipeline) {
		m_CurrentPipeline = inPipeline;
		CVulkanResourceManager::bindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, inPipeline->mPipeline);
		CVulkanResourceManager::bindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, inPipeline->mLayout, 0, 1, CVulkanResourceManager::getBindlessDescriptorSet());

		//TODO: shouldnt do this here...
		Extent32u extent = CEngineViewport::get()->mExtent;

		VkViewport viewport = {};
		viewport.x = 0;
		viewport.y = 0;
		viewport.width = (float)extent.x;
		viewport.height = (float)extent.y;
		viewport.minDepth = 0.f;
		viewport.maxDepth = 1.f;

		vkCmdSetViewport(cmd, 0, 1, &viewport);

		VkRect2D scissor = {};
		scissor.offset.x = 0;
		scissor.offset.y = 0;
		scissor.extent.width = (float)extent.x;
		scissor.extent.height = (float)extent.y;

		vkCmdSetScissor(cmd, 0, 1, &scissor);
	}
	//}

	vkCmdPushConstants(cmd, inPipeline->mLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(SPushConstants), inConstants.data());

}

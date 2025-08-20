#include "passes/Pass.h"

#include "Material.h"
#include "Viewport.h"
#include "renderer/VulkanRenderer.h"
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
	CVulkanRenderer::get()->immediateSubmit([&](SCommandBuffer cmd) {
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
		CEngineViewport::get().set(cmd);
	}
	//}

	vkCmdPushConstants(cmd, inPipeline->mLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(SPushConstants), inConstants.data());

}

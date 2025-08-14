#include "Pass.h"

#include "Engine.h"
#include "Material.h"
#include "Viewport.h"
#include "VulkanUtils.h"
#include "VulkanResourceManager.h"

void CPass::bindPipeline(const VkCommandBuffer cmd, CPipeline* inPipeline, const SPushConstants& inConstants) {
	//if (surface.material != lastMaterial) {
	//lastMaterial = surface.material;
	// If the pipeline has changed, rebind pipeline data
	if (inPipeline != m_CurrentPipeline) {
		m_CurrentPipeline = inPipeline;
		CVulkanResourceManager::bindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, inPipeline->mPipeline);
		CVulkanResourceManager::bindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, inPipeline->mLayout, 0, 1, CVulkanResourceManager::getBindlessDescriptorSet());

		//TODO: shouldnt do this here...
		CEngine::get().getViewport().set(cmd);
	}
	//}

	vkCmdPushConstants(cmd, inPipeline->mLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(SPushConstants), inConstants.data());

}

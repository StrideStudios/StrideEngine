#include "passes/Pass.h"

#include "Material.h"
#include "Viewport.h"
#include "renderer/EngineTextures.h"
#include "renderer/VulkanRenderer.h"

void CPass::beginRendering(VkCommandBuffer cmd, const Extent32u inExtent, const CEngineTextures& inEngineTextures) const {
	VkRenderingAttachmentInfo colorAttachment = getColorAttachment().get(inEngineTextures.mDrawImage);
	VkRenderingAttachmentInfo depthAttachment = getDepthAttachment().get(inEngineTextures.mDepthImage);
	VkRenderingAttachmentInfo stencilAttachment = getStencilAttachment().get(nullptr);

	VkRenderingInfo renderInfo {
		.sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
		.pNext = nullptr,
		.renderArea = VkRect2D {
			VkOffset2D {0,0},
			VkExtent2D {inExtent.x, inExtent.y}
		},
		.layerCount = 1,
		.viewMask = 0,
		.colorAttachmentCount = 1
	};

	if (colorAttachment.loadOp != VK_ATTACHMENT_LOAD_OP_NONE || colorAttachment.storeOp != VK_ATTACHMENT_STORE_OP_NONE) {
		renderInfo.pColorAttachments = &colorAttachment;
	}
	if (depthAttachment.loadOp != VK_ATTACHMENT_LOAD_OP_NONE || depthAttachment.storeOp != VK_ATTACHMENT_STORE_OP_NONE) {
		renderInfo.pDepthAttachment = &depthAttachment;
	}
	if (stencilAttachment.loadOp != VK_ATTACHMENT_LOAD_OP_NONE || stencilAttachment.storeOp != VK_ATTACHMENT_STORE_OP_NONE) {
		renderInfo.pStencilAttachment = &stencilAttachment;
	}

	vkCmdBeginRendering(cmd, &renderInfo);
}

bool CPass::hasSameRenderingInfo(const CPass* inOther) const {
	return getRenderingInfoFlags() == inOther->getRenderingInfoFlags() &&
		isResolvePass() == inOther->isResolvePass() &&
		getColorAttachment() == inOther->getColorAttachment() &&
		getDepthAttachment() == inOther->getDepthAttachment() &&
		getStencilAttachment() == inOther->getStencilAttachment();
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

#include "Pass.h"

#include "BindlessResources.h"
#include "Material.h"

void CPass::beginRendering(VkCommandBuffer cmd, const Extent32u inExtent, const SImage_T* inColorImage, const SImage_T* inDepthImage, const SImage_T* inStencilImage) const {
	VkRenderingAttachmentInfo colorAttachment = getColorAttachment().get(inColorImage);
	VkRenderingAttachmentInfo depthAttachment = getDepthAttachment().get(inDepthImage);
	VkRenderingAttachmentInfo stencilAttachment = getStencilAttachment().get(inStencilImage);

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
	// If the pipeline has changed, rebind pipeline data
	//if (inPipeline != m_CurrentPipeline) {
		m_CurrentPipeline = inPipeline;
		inPipeline->bind(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS);
		CBindlessResources::getBindlessDescriptorSet()->bind(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, inPipeline->mLayout->mPipelineLayout, 0, 1);
	//}

	vkCmdPushConstants(cmd, inPipeline->mLayout->mPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(SPushConstants), inConstants.data());

}

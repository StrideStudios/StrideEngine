#include "rendercore/Pass.h"

#include "VRI/VRICommands.h"
#include "VRI/BindlessResources.h"
#include "rendercore/Material.h"

void CPass::beginRendering(const TFrail<CVRICommands>& cmd, const Extent32u inExtent, const TFrail<SVRIImage>& inColorImage, const TFrail<SVRIImage>& inDepthImage, const TFrail<SVRIImage>& inStencilImage) const {
	const VkRenderingAttachmentInfo colorAttachment = getColorAttachment().get(inColorImage.get());
	const VkRenderingAttachmentInfo depthAttachment = getDepthAttachment().get(inDepthImage.get());
	const VkRenderingAttachmentInfo stencilAttachment = getStencilAttachment().get(inStencilImage.get());

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

	cmd->beginRendering(renderInfo);
}

bool CPass::hasSameRenderingInfo(const CPass* inOther) const {
	return getRenderingInfoFlags() == inOther->getRenderingInfoFlags() &&
		isResolvePass() == inOther->isResolvePass() &&
		getColorAttachment() == inOther->getColorAttachment() &&
		getDepthAttachment() == inOther->getDepthAttachment() &&
		getStencilAttachment() == inOther->getStencilAttachment();
}

void CPass::bindPipeline(const TFrail<CVRICommands>& cmd, CPipeline* inPipeline, const SPushConstants& inConstants) {
	// If the pipeline has changed, rebind pipeline data
	//if (inPipeline != m_CurrentPipeline) {
		m_CurrentPipeline = inPipeline;
		cmd->bindPipeline(inPipeline, VK_PIPELINE_BIND_POINT_GRAPHICS);
		cmd->bindDescriptorSets(CBindlessResources::getBindlessDescriptorSet(), VK_PIPELINE_BIND_POINT_GRAPHICS, inPipeline->mLayout->mPipelineLayout, 0, 1);
	//}

	cmd->pushConstants(inPipeline->mLayout->mPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(SPushConstants), inConstants.data());

}

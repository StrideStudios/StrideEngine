#include "Material.h"

#include "MeshPass.h"
#include "VulkanRenderer.h"

SMaterialPipeline& CMaterial::getPipeline(const CVulkanRenderer& renderer) const {
	switch (mPassType) {
		case EMaterialPass::OPAQUE:
			return renderer.mBasePass->opaquePipeline;
		case EMaterialPass::TRANSLUCENT:
			return renderer.mBasePass->transparentPipeline;
		default:
			return renderer.mBasePass->errorPipeline;
	}
}

#include "renderer/Material.h"

#include "passes/MeshPass.h"
#include "renderer/VulkanRenderer.h"

CPipeline* CMaterial::getPipeline(const CVulkanRenderer& renderer) const {
	switch (mPassType) {
		case EMaterialPass::OPAQUE:
			return renderer.mBasePass->opaquePipeline;
		case EMaterialPass::TRANSLUCENT:
			return renderer.mBasePass->transparentPipeline;
		case EMaterialPass::WIREFRAME:
			return renderer.mBasePass->wireframePipeline;
		default:
			return renderer.mBasePass->errorPipeline;
	}
}

#include "Material.h"

#include "GpuScene.h"
#include "MeshPass.h"
#include "VulkanRenderer.h"

SMaterialPipeline& CMaterial::getPipeline(const CVulkanRenderer& renderer) const {
	switch (mPassType) {
		case EMaterialPass::OPAQUE:
			return renderer.mGPUScene->basePass->opaquePipeline;
		case EMaterialPass::TRANSLUCENT:
			return renderer.mGPUScene->basePass->transparentPipeline;
		default:
			return renderer.mGPUScene->basePass->errorPipeline;
	}
}

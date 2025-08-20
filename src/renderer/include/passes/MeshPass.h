#pragma once

#include "Material.h"
#include "passes/Pass.h"

enum class EMeshPass : uint8 {
	BASE_PASS
};

class CVulkanRenderer;
class CGPUScene;

class CMeshPass : public CPass, public IInitializable<EMeshPass> {

public:

	CMeshPass(): CPass("MeshPass") {}

	virtual void init(EMeshPass inPassType) override;

	virtual void render(VkCommandBuffer cmd) override;

	EMeshPass passType = EMeshPass::BASE_PASS;

	//
	// Pipelines
	//

	CPipeline* opaquePipeline = nullptr;
	CPipeline* errorPipeline = nullptr;
	CPipeline* wireframePipeline = nullptr;
	CPipeline* transparentPipeline = nullptr;
};
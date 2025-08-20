#pragma once

#include "passes/Pass.h"

enum class EMeshPass : uint8 {
	BASE_PASS
};

class CVulkanRenderer;
class CGPUScene;

class EXPORT CMeshPass : public CPass {

public:

	DEFINE_PASS(CMeshPass)

	virtual void init() override;

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
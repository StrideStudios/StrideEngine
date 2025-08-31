#pragma once

#include "Pass.h"

enum class EMeshPass : uint8 {
	BASE_PASS
};

class CVulkanRenderer;
class CGPUScene;

class CMeshPass : public CPass {

public:

	DEFINE_PASS(CMeshPass)

	EXPORT virtual void init() override;

	EXPORT virtual void render(VkCommandBuffer cmd) override;

	EMeshPass passType = EMeshPass::BASE_PASS;

	//
	// Pipelines
	//

	CPipeline* opaquePipeline = nullptr;
	CPipeline* errorPipeline = nullptr;
	CPipeline* wireframePipeline = nullptr;
	CPipeline* transparentPipeline = nullptr;
};
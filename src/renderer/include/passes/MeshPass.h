#pragma once

#include "Pass.h"

class CVulkanRenderer;

class CMeshPass : public CPass {

	REGISTER_PASS(CMeshPass)

public:

	EXPORT virtual void init() override;

	EXPORT virtual void render(VkCommandBuffer cmd) override;

	//
	// Pipelines
	//

	CPipeline* opaquePipeline = nullptr;
	CPipeline* errorPipeline = nullptr;
	CPipeline* wireframePipeline = nullptr;
	CPipeline* transparentPipeline = nullptr;
};
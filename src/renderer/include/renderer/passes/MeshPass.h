#pragma once

#include "rendercore/Pass.h"

class CVulkanRenderer;

class CMeshPass : public CPass {

	REGISTER_PASS(CMeshPass, CPass)

public:

	EXPORT virtual void init(CRenderer* inRenderer) override;

	EXPORT virtual void render(VkCommandBuffer cmd) override;

	//
	// Pipelines
	//

	CPipeline* opaquePipeline = nullptr;
	CPipeline* errorPipeline = nullptr;
	CPipeline* wireframePipeline = nullptr;
	CPipeline* transparentPipeline = nullptr;
};
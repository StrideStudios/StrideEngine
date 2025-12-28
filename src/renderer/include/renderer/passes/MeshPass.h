#pragma once

#include "rendercore/Pass.h"

class CVulkanRenderer;

class CMeshPass : public CPass {

	REGISTER_PASS(CMeshPass, CPass)

public:

	EXPORT virtual void init(TShared<CRenderer> inRenderer) override;

	EXPORT void destroy() override;

	EXPORT virtual void render(const SRendererInfo& info, VkCommandBuffer cmd) override;

	//
	// Pipelines
	//

	TUnique<CPipeline> opaquePipeline = nullptr;
	TUnique<CPipeline> errorPipeline = nullptr;
	TUnique<CPipeline> wireframePipeline = nullptr;
	TUnique<CPipeline> transparentPipeline = nullptr;
};
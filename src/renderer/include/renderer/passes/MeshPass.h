#pragma once

#include "rendercore/Pass.h"

class CVulkanRenderer;

class CMeshPass : public CPass {

	REGISTER_PASS(CMeshPass, CPass)

public:

	EXPORT virtual void init(TFrail<CRenderer> inRenderer) override;

	EXPORT void destroy() override;

	EXPORT virtual void render(const SRendererInfo& info, const TFrail<CVRICommands>& cmd) override;

	//
	// Pipelines
	//

	TUnique<SPipeline> opaquePipeline = nullptr;
	TUnique<SPipeline> errorPipeline = nullptr;
	TUnique<SPipeline> wireframePipeline = nullptr;
	TUnique<SPipeline> transparentPipeline = nullptr;
};
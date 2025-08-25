#pragma once

#include <set>

#include "renderer/EngineLoader.h"
#include "Pass.h"

class CSprite;

class EXPORT CEngineUIPass : public CPass {

public:

	DEFINE_PASS(CEngineUIPass)

	virtual void init() override;

	virtual void render(VkCommandBuffer cmd) override;

	// Sprites used to render this pass
	std::set<std::shared_ptr<CSprite>> objects;

	//
	// Pipelines
	//

	CPipeline* opaquePipeline = nullptr;
};

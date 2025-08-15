#pragma once

#include <set>

#include "../../core/include/EngineLoader.h"
#include "Material.h"
#include "Pass.h"

class CSprite;

class CEngineUIPass : public CPass, public IInitializable<> {

public:

	virtual void init() override;

	virtual void render(VkCommandBuffer cmd) override;

	// Sprites used to render this pass
	std::set<std::shared_ptr<CSprite>> objects;

	//
	// Pipelines
	//

	CPipeline* opaquePipeline;
};

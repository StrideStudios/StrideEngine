#pragma once

#include <set>

#include "Material.h"
#include "Pass.h"

class CSprite;

class CSpritePass : public CPass, public IInitializable<> {

public:

	virtual void init() override;

	virtual void render(VkCommandBuffer cmd) override;

	void push(const std::set<std::shared_ptr<CSprite>>& inObjects) {
		objects.insert_range(inObjects);
	}

	void push(const std::shared_ptr<CSprite>& inObject) {
		objects.insert(inObject);
	}

	// Sprited used to render this pass
	std::set<std::shared_ptr<CSprite>> objects;

	//
	// Pipelines
	//

	SMaterialPipeline opaquePipeline;
};

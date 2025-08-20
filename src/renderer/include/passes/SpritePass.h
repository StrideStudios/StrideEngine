#pragma once

#include <set>

#include "renderer/EngineLoader.h"
#include "Pass.h"

class CSprite;

class EXPORT CSpritePass : public CPass {

public:

	CSpritePass(): CPass("SpritePass") {}

	virtual void init() override;

	virtual void render(VkCommandBuffer cmd) override;

	virtual void destroy() override {
		objects.clear();
	}

	void push(const std::set<std::shared_ptr<CSprite>>& inObjects) {
		for (auto& sprite : inObjects) {
			push(sprite);
		}
	}

	void push(const std::shared_ptr<CSprite>& inObject) {
		objects.insert(inObject);
	}

	// Sprites used to render this pass
	std::set<std::shared_ptr<CSprite>> objects;

	//
	// Pipelines
	//

	CPipeline* opaquePipeline = nullptr;
};

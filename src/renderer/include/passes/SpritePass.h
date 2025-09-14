#pragma once

#include <set>

#include "renderer/EngineLoader.h"
#include "Pass.h"

class CSprite;

class CSpritePass : public CPass {

	REGISTER_PASS(CSpritePass)

public:

	EXPORT virtual void init() override;

	EXPORT virtual void render(VkCommandBuffer cmd) override;

	EXPORT virtual void update() override;

	EXPORT virtual void destroy() override;

	void push(const std::set<std::shared_ptr<CSprite>>& inObjects) {
		for (auto& sprite : inObjects) {
			push(sprite);
		}
	}

	void push(const std::shared_ptr<CSprite>& inObject) {
		objects.insert(inObject);
	}

	// Sprites used to render this pass
	std::set<std::shared_ptr<CSprite>> objects{};

	//
	// Pipelines
	//

	CPipeline* opaquePipeline = nullptr;
};

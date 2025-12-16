#pragma once

#include <set>

#include "rendercore/EngineLoader.h"
#include "rendercore/Pass.h"

class CRenderableViewportObject;

class CSpritePass : public CPass {

	REGISTER_PASS(CSpritePass, CPass)

public:

	EXPORT virtual void init() override;

	EXPORT virtual void render(VkCommandBuffer cmd) override;

	EXPORT virtual void update() override;

	EXPORT virtual void destroy() override;

	void push(const std::set<std::shared_ptr<CRenderableViewportObject>>& inObjects) {
		for (auto& sprite : inObjects) {
			push(sprite);
		}
	}

	void push(const std::shared_ptr<CRenderableViewportObject>& inObject) {
		objects.insert(inObject);
	}

	// Sprites used to render this pass
	std::set<std::shared_ptr<CRenderableViewportObject>> objects{};

	//
	// Pipelines
	//

	CPipeline* opaquePipeline = nullptr;
};

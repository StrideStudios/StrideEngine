#pragma once

#include "passes/SpritePass.h"
#include "renderer/VulkanRenderer.h"

class CEditorSpritePass : public CSpritePass {

public:

	DEFINE_PASS(CEditorSpritePass)

	EXPORT virtual void init() override;

	EXPORT virtual void render(VkCommandBuffer cmd) override;

	//
	// Pipelines
	//

	CPipeline* textPipeline = nullptr;
};

class CEditorRenderer :public CVulkanRenderer {

public:

	EXPORT virtual void init() override;

	EXPORT virtual void destroy() override;
};
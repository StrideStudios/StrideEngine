#pragma once

#include "renderer/passes/SpritePass.h"
#include "renderer/VulkanRenderer.h"

class CEditorSpritePass : public CSpritePass {

	REGISTER_PASS(CEditorSpritePass, CSpritePass)

public:

	EXPORT virtual void init(TFrail<CRenderer> inRenderer) override;

	EXPORT virtual void render(const SRendererInfo& info, VkCommandBuffer cmd) override;

	EXPORT virtual void destroy() override;

	//
	// Pipelines
	//

	TUnique<CPipeline> textPipeline = nullptr;

	TUnique<CMaterial> textMaterial = nullptr;

	SDynamicBuffer<VMA_MEMORY_USAGE_CPU_TO_GPU, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT> tempTextBuffer{"Text Buffer"};
};

class CEditorRenderer : public CVulkanRenderer {

public:

	EXPORT virtual void init() override;

	EXPORT virtual void destroy() override;
};
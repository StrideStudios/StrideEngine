#pragma once

#include "vulkan/vulkan_core.h"

struct SEngineUI {

	static void begin();

	static void init(VkQueue inQueue, VkFormat format);

	static void destroy();

	static void renderSceneUI();

	static void renderTextureUI();

	static void renderMaterialUI();

	static void renderSpriteUI();

	static void renderMeshUI();

	static void render(VkCommandBuffer cmd);

};

#pragma once

#include "Material.h"

struct SEngineUI {

	static void begin();

	static void init(VkQueue inQueue, VkFormat format);

	static void destroy();

	static void render(VkCommandBuffer cmd, VkExtent2D inExtent, VkImageView inTargetImageView);

	static void renderSceneUI();

	static void renderTextureUI();

	static void renderMaterialUI();

	static void renderSpriteUI();

	static void renderMeshUI();

	static void runEngineUI();
};

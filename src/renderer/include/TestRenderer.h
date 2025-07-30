#pragma once

#include "VulkanRenderer.h"

class CTestRenderer : public CVulkanRenderer {
	void init() override;

	void render(VkCommandBuffer cmd) override;
};

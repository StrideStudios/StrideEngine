#pragma once

#include "VulkanRenderer.h"

class CTestRenderer : public CVulkanRenderer {
public:
	virtual void init() override;

	virtual void destroy() override;

	virtual void render(VkCommandBuffer cmd) override;

};

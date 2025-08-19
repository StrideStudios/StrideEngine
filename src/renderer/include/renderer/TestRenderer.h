#pragma once

#include "VulkanRenderer.h"

class EXPORT CTestRenderer : public CVulkanRenderer {
public:
	virtual void init() override;

	virtual void destroy() override;

	virtual void render(VkCommandBuffer cmd) override;

};

#pragma once

#include "VulkanRenderer.h"

class CBaseRenderer : public CVulkanRenderer {

public:

	CBaseRenderer() = default;

	void init() override;

	void render(VkCommandBuffer cmd) override;

private:

	//
	// Pipelines
	//

	VkPipelineLayout m_GradientPipelineLayout = nullptr;

	std::vector<SComputeEffect> m_BackgroundEffects;
};

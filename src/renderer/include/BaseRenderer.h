#pragma once

#include "VulkanRenderer.h"

class CBaseRenderer : public CVulkanRenderer {

public:

	CBaseRenderer();

	void render(VkCommandBuffer cmd) override;

private:

	//
	// Pipelines
	//

	VkPipelineLayout m_GradientPipelineLayout = nullptr;

	std::vector<SComputeEffect> m_BackgroundEffects;
};

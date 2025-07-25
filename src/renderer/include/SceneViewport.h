#pragma once

#include <vulkan/vulkan_core.h>

#include "BasicTypes.h"

struct SSceneViewport {

	SSceneViewport() = default;

	void set(const VkCommandBuffer cmd) const {
		VkViewport viewport = {};
		viewport.x = 0;
		viewport.y = 0;
		viewport.width = m_Extent.x;
		viewport.height = m_Extent.y;
		viewport.minDepth = 0.f;
		viewport.maxDepth = 1.f;

		vkCmdSetViewport(cmd, 0, 1, &viewport);

		VkRect2D scissor = {};
		scissor.offset.x = 0;
		scissor.offset.y = 0;
		scissor.extent.width = m_Extent.x;
		scissor.extent.height = m_Extent.y;

		vkCmdSetScissor(cmd, 0, 1, &scissor);
	}

	void update(Extent32u inExtent) {
		m_Extent = inExtent;
	}

	Extent32u getExtent() const { return m_Extent; }

private:

	Extent32u m_Extent;
};

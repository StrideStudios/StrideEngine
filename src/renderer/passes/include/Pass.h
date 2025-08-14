#pragma once

#include <vulkan/vulkan_core.h>

#include "ResourceManager.h"

class CPipeline;

class CPass : public IDestroyable {

public:

	virtual void render(VkCommandBuffer cmd) = 0;

	void bindPipeline(VkCommandBuffer cmd, CPipeline* inPipeline, const struct SPushConstants& inConstants);

private:

	CPipeline* m_CurrentPipeline = nullptr;
};

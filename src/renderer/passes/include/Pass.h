#pragma once

#include <vulkan/vulkan_core.h>

#include "ResourceManager.h"

class CPass : public IDestroyable {

	virtual void render(VkCommandBuffer cmd) = 0;
};

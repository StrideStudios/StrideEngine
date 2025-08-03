#pragma once
#include "ResourceManager.h"

class CPass : public IDestroyable {

	virtual void render(VkCommandBuffer cmd) = 0;
};

#pragma once

#include "Common.h"
#include <vulkan/vulkan_core.h>

struct SMaterialPipeline {
	VkPipeline pipeline;
	VkPipelineLayout layout;
};

enum class EMaterialPass : uint8 {
	OPAQUE,
	TRANSLUCENT,
	HIGHLIGHT
};

struct SMaterialInstance {
	SMaterialPipeline* pipeline;
	EMaterialPass passType;
	int32 colorTextureId = 0;
};
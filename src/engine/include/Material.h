#pragma once

#include "Common.h"
#include <vulkan/vulkan_core.h>

enum class EMaterialPass : uint8 {
	OPAQUE,
	TRANSLUCENT
};

struct SMaterialPipeline {
	VkPipeline pipeline;
	VkPipelineLayout layout;
};

struct SMaterialInstance {
	SMaterialPipeline* pipeline;
	VkDescriptorSet materialSet;
	EMaterialPass passType;
};

struct GLTFMaterial {
	SMaterialInstance data;
};
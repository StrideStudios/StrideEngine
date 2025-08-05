#pragma once

#include <vector>
#include <string>
#include <vulkan/vulkan_core.h>
#include "Common.h"

enum class EShaderStage : uint8 {
	VERTEX,
	PIXEL,
	COMPUTE
};

struct SShader {
	VkShaderModule mModule;
	EShaderStage mStage;
	std::string mShaderCode;
	std::vector<uint32> mCompiledShader;
};

class CShaderCompiler {

public:

	static VkResult getShader(VkDevice inDevice, const char* inFileName, SShader& inoutShader);

};

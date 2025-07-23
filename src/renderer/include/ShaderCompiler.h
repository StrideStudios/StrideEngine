#pragma once

#include <vector>
#include <vulkan/vulkan_core.h>
#include "BasicTypes.h"

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

	static VkResult compileShader(VkDevice inDevice, const char* inFileName, SShader& inoutShader);

	static bool loadShader(const char* inFilePath, VkDevice inDevice, VkShaderModule* outShaderModule);
};

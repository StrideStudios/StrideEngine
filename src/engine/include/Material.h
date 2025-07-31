#pragma once

#include <vulkan/vulkan_core.h>
#include <set>

#include "Archive.h"
#include "Common.h"

struct SMaterialPipeline {
	VkPipeline pipeline;
	VkPipelineLayout layout;
};

enum class EMaterialPass : uint8 {
	OPAQUE,
	TRANSLUCENT,
	HIGHLIGHT
};
ENUM_OPERATORS(EMaterialPass, uint8)

struct SMaterialInstance {
	SMaterialPipeline* pipeline;
	EMaterialPass passType;
	int32 colorTextureId = 0;
};

class CMaterial {
public:

	static std::set<SMaterialInstance> getMaterials() {
		static std::set<SMaterialInstance> materials;
		return materials;
	}

	CMaterial() = default;

	int32 testNum0 = 0;
	int32 testNum1 = 0;

	EMaterialPass mPassType = EMaterialPass::OPAQUE;

	// The code for the file
	// This is compiled into glsl, then to spirv
	std::string mCode;

	Vector3f otherNum = {0, 0, 0};

	//TODO: for now this form of saving and loading should suffice
	// but before going any further, a class with a standard c implementation should be used

	friend CArchive& operator<<(CArchive& inArchive, const CMaterial& inMaterial) {
		inArchive << inMaterial.testNum0;
		inArchive << inMaterial.testNum1;
		inArchive << inMaterial.mPassType;

		// Write the size so we know how much to read
		inArchive << inMaterial.mCode;
		inArchive << inMaterial.otherNum;
		return inArchive;
	}

	friend CArchive& operator>>(CArchive& inArchive, CMaterial& inMaterial) {
		inArchive >> inMaterial.testNum0;
		inArchive >> inMaterial.testNum1;
		inArchive >> inMaterial.mPassType;

		// Get size and read the file to the code
		inArchive >> inMaterial.mCode;
		inArchive >> inMaterial.otherNum;
		return inArchive;
	}
};
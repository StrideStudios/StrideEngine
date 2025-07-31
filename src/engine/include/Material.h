#pragma once

#include <vulkan/vulkan_core.h>
#include <map>

#include "Archive.h"
#include "Common.h"

struct SMaterialPipeline {
	VkPipeline pipeline;
	VkPipelineLayout layout;
};

enum class EMaterialPass : uint8 {
	OPAQUE,
	TRANSLUCENT,
	HIGHLIGHT,
	MAX
};
ENUM_TO_STRING(MaterialPass, uint8,
	"Opaque",
	"Translucent",
	"Highlight");
ENUM_OPERATORS(MaterialPass, uint8)

struct SMaterialInstance {
	SMaterialPipeline* pipeline;
	EMaterialPass passType;
	int32 colorTextureId = 0;
};

class CMaterial {
public:

	static std::vector<CMaterial>& getMaterials() {
		static std::vector<CMaterial> materials;
		return materials;
	}

	CMaterial() = default;

	std::string mName;

	EMaterialPass mPassType = EMaterialPass::OPAQUE;

	// Inputs for the file, these are put into a uniform buffer
	// which always has 16 Vector4fs
	std::vector<Vector4f> mInputs;

	// The code for the file
	// This is compiled into glsl, then to spirv
	//TODO: actually write the compiler and stuff
	std::string mCode;

	//TODO: for now this form of saving and loading should suffice
	// but before going any further, a class with a standard c implementation should be used

	friend CArchive& operator<<(CArchive& inArchive, const CMaterial& inMaterial) {
		inArchive << inMaterial.mName;
		inArchive << inMaterial.mPassType;
		inArchive << inMaterial.mInputs;
		inArchive << inMaterial.mCode;
		return inArchive;
	}

	friend CArchive& operator>>(CArchive& inArchive, CMaterial& inMaterial) {
		inArchive >> inMaterial.mName;
		inArchive >> inMaterial.mPassType;
		inArchive >> inMaterial.mInputs;
		inArchive >> inMaterial.mCode;
		return inArchive;
	}
};
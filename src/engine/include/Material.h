#pragma once

#include <vulkan/vulkan_core.h>
#include <map>
#include <array>

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
	ERROR,
	MAX
};
ENUM_TO_STRING(MaterialPass, uint8,
	"Opaque",
	"Translucent",
	"Highlight",
	"Error");
ENUM_OPERATORS(MaterialPass, uint8)

// Push constants allow for a max of 128 bytes (modern hardware can do 256, but is not guaranteed)
// This means you can push 32 floating point values
// Since these are used to access the bindless textures, that means a max of 32 textures, or 64 if double packed
// If i wish to double pack i need to make sure max textures is uint16

//TODO: ensure this, for now 32 works but having more textures would be more better
static uint16 gMaxTextures = std::numeric_limits<uint16>::max();

struct SPushConstants {
	Vector4f mConstants[8];
};

struct SMaterialInstance {
	SMaterialPipeline* pipeline;
	EMaterialPass passType;
	SPushConstants constants;
};

class CMaterial {
public:

	static std::vector<std::shared_ptr<CMaterial>>& getMaterials() {
		static std::vector<std::shared_ptr<CMaterial>> materials;
		return materials;
	}

	CMaterial() {
		mConstants.fill({});
	}

	SMaterialPipeline& getPipeline(const class CVulkanRenderer* renderer) const;

	bool mShouldSave = true;

	std::string mName;

	EMaterialPass mPassType = EMaterialPass::OPAQUE;

	// Inputs for the file, these are put into a uniform buffer
	// which always has 16 Vector4fs
	std::vector<Vector4f> mInputs;

	// The code for the file
	// This is compiled into glsl, then to spirv
	//TODO: actually write the compiler and stuff
	std::string mCode;

	std::array<Vector4f, 8> mConstants;

	//TODO: for now this form of saving and loading should suffice
	// but before going any further, a class with a standard c implementation should be used

	friend CArchive& operator<<(CArchive& inArchive, const CMaterial& inMaterial) {
		if (inMaterial.mShouldSave) {
			inArchive << inMaterial.mName;
			inArchive << inMaterial.mPassType;
			inArchive << inMaterial.mInputs;
			inArchive << inMaterial.mCode;
			inArchive << inMaterial.mConstants;
		}
		return inArchive;
	}

	friend CArchive& operator>>(CArchive& inArchive, CMaterial& inMaterial) {
		if (inMaterial.mShouldSave) {
			inArchive >> inMaterial.mName;
			inArchive >> inMaterial.mPassType;
			inArchive >> inMaterial.mInputs;
			inArchive >> inMaterial.mCode;
			inArchive >> inMaterial.mConstants;
		}
		return inArchive;
	}
};
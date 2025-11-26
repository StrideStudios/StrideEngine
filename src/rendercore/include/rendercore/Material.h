#pragma once

#include <array>

#include "basic/core/Hashing.h"

enum class EMaterialPass : uint8 {
	OPAQUE,
	TRANSLUCENT,
	ERROR,
	WIREFRAME,
	MAX
};
ENUM_TO_STRING(MaterialPass, uint8,
	"Opaque",
	"Translucent",
	"Error");
ENUM_OPERATORS(MaterialPass, uint8)

// Push constants allow for a max of 128 bytes (modern hardware can do 256, but is not guaranteed)
// This means you can push 32 floating point values
// Since these are used to access the bindless textures, that means a max of 32 textures, or 64 if double packed
// If i wish to double pack i need to make sure max textures is uint16

// Safe array for push constants that is always filled by default
struct SPushConstants : std::array<Vector4f, 8> {
	SPushConstants() : array() {
		fill(Vector4f(0.f));
	}
};

class CMaterial : public SObject {

	REGISTER_CLASS(CMaterial, SObject)

public:

	CMaterial() = default;

	bool mShouldSave = true;

	std::string mName;

	EMaterialPass mPassType = EMaterialPass::OPAQUE;

	// The code for the file
	// This is compiled into glsl, then to spirv
	//TODO: actually write the compiler and stuff
	std::string mCode;

	// The push constants for this material
	// They are always in the form of floating point values, but can be multipurpose
	SPushConstants mConstants;

	//TODO: for now this form of saving and loading should suffice
	// but before going any further, a class with a standard c implementation should be used

	friend CArchive& operator<<(CArchive& inArchive, const CMaterial& inMaterial) {
		if (inMaterial.mShouldSave) {
			inArchive << getHash(inMaterial.mName);
			inArchive << inMaterial.mName;
			inArchive << inMaterial.mPassType;
			inArchive << inMaterial.mCode;
			inArchive << inMaterial.mConstants;
		}
		return inArchive;
	}

	friend CArchive& operator>>(CArchive& inArchive, CMaterial& inMaterial) {
		if (inMaterial.mShouldSave) {
			int32 hash;
			inArchive >> hash;//TODO: hash
			inArchive >> inMaterial.mName;
			inArchive >> inMaterial.mPassType;
			inArchive >> inMaterial.mCode;
			inArchive >> inMaterial.mConstants;
		}
		return inArchive;
	}
};
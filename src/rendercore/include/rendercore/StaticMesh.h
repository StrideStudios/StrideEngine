#pragma once

#include <memory>
#include <string>
#include <vector>

#include "rendercore/Material.h"
#include "rendercore/VulkanResources.h"

struct SBounds {
	Vector3f origin;
	float sphereRadius;
	Vector3f extents;

	friend CArchive& operator<<(CArchive& inArchive, const SBounds& inBounds) {
		inArchive << inBounds.origin;
		inArchive << inBounds.sphereRadius;
		inArchive << inBounds.extents;
		return inArchive;
	}

	friend CArchive& operator>>(CArchive& inArchive, SBounds& inBounds) {
		inArchive >> inBounds.origin;
		inArchive >> inBounds.sphereRadius;
		inArchive >> inBounds.extents;
		return inArchive;
	}
};

struct SStaticMesh : public SObject {

	REGISTER_STRUCT(SStaticMesh, SObject)

	struct Surface {

		std::string name;

		CMaterial* material;

		uint32 startIndex;
		uint32 count;
	};

	std::string name{"None"};

	SBounds bounds;
	std::vector<Surface> surfaces;
	TUnique<SVRIMeshBuffer> meshBuffers;

	friend uint32 getHash(const SStaticMesh& inMesh) {
		return getHash(inMesh.name);
	}

	friend CArchive& operator<<(CArchive& inArchive, const SStaticMesh& inMesh) {
		inArchive << inMesh.name;
		return inArchive;
	}

	friend CArchive& operator>>(CArchive& inArchive, SStaticMesh& inMesh) {
		inArchive >> inMesh.name;
		return inArchive;
	}
};
#pragma once

#include <memory>
#include <string>
#include <vector>

#include "Material.h"
#include "VulkanResources.h"

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

struct SStaticMesh {

	struct Surface {

		std::string name;

		std::shared_ptr<CMaterial> material;

		uint32 startIndex;
		uint32 count;
	};

	std::string name{"None"};

	SBounds bounds;
	std::vector<Surface> surfaces;
	SMeshBuffers_T* meshBuffers;

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
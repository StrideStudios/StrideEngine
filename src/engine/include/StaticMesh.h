#pragma once

#include <memory>
#include <string>
#include <vector>

#include "BasicTypes.h"
#include "Material.h"
#include "SceneObject.h"

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

	std::string name;

	SBounds bounds;
	std::vector<Surface> surfaces;
	SMeshBuffers meshBuffers;
};

class CStaticMeshObject : public CRenderableObject {

public:

	std::shared_ptr<SStaticMesh> getMesh() override {
		return mesh;
	}

	std::shared_ptr<SStaticMesh> mesh;
};
#pragma once

#include <memory>
#include <string>
#include <vector>

#include "BasicTypes.h"
#include "Material.h"

struct SBounds {
	Vector3f origin;
	float sphereRadius;
	Vector3f extents;
};

struct SStaticMesh {

	struct Surface {
		uint32 startIndex;
		uint32 count;
		std::shared_ptr<GLTFMaterial> material;
		SBounds bounds;
	};

	std::string name;

	std::vector<Surface> surfaces;
	SMeshBuffers meshBuffers;
};

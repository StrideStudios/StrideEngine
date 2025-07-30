#pragma once

#include <memory>
#include <string>
#include <vector>

#include "BasicTypes.h"

struct SBounds {
	Vector3f origin;
	float sphereRadius;
	Vector3f extents;
};

struct SStaticMesh {

	struct Surface {

		std::string name;

		std::shared_ptr<SMaterialInstance> material;

		uint32 startIndex;
		uint32 count;
	};

	std::string name;

	SBounds bounds;
	Matrix4f transform;
	std::vector<Surface> surfaces;
	SMeshBuffers meshBuffers;
};

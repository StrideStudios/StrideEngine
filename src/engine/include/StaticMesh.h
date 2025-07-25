#pragma once

#include <memory>
#include <string>
#include <vector>

#include "BasicTypes.h"
#include "Material.h"

struct SStaticMesh {

	struct Surface {
		uint32 startIndex;
		uint32 count;
		std::shared_ptr<GLTFMaterial> material;
	};

	std::string name;

	std::vector<Surface> surfaces;
	SMeshBuffers meshBuffers;
};

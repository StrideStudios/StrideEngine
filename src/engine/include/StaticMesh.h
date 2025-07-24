#pragma once

#include <memory>
#include <string>
#include <vector>

#include "BasicTypes.h"
#include "ResourceAllocator.h"

struct SStaticMesh {

	struct Surface {
		uint32 startIndex;
		uint32 count;
	};

	std::string name;

	std::vector<Surface> surfaces;
	SMeshBuffers meshBuffers;
};

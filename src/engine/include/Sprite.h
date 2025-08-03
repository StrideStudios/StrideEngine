#pragma once

#include <memory>
#include <string>

#include "BasicTypes.h"

struct SSprite {

	struct Data {
		Vector2f mUV0;
		Vector2f mUV1;
		Vector4f mColor;
	};

	std::string name;

	// Surface Data
	std::shared_ptr<CMaterial> material;
};

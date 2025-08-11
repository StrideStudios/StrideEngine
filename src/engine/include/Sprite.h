#pragma once

#include <memory>

#include "Common.h"
#include "Widget.h"

class CSprite : public CWidget {

public:

	struct Data {
		Vector2f mUV0;
		Vector2f mUV1;
		Vector4f mColor;
	};

	// Surface Data
	std::shared_ptr<CMaterial> material;
};

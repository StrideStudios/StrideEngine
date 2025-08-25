#pragma once

#include <set>

#include "renderer/EngineLoader.h"
#include "SpritePass.h"

class CSprite;

class EXPORT CEngineUIPass : public CSpritePass {

public:

	DEFINE_PASS(CEngineUIPass)

	virtual void init() override;

};

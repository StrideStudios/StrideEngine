#pragma once

#include "basic/core/Common.h"

struct SEngineTime : SBase {

	MAKE_SINGLETON(SEngineTime)

public:

	int32 mAverageFrameRate = 0;
	int32 mFrameRate = 0;
	double mDeltaTime = 0.0;
	double mGameTime = 0.0;
};

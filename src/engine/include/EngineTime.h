#pragma once

#include "core/Common.h"

struct SEngineTime {

	EXPORT static SEngineTime& get();

	int32 mAverageFrameRate = 0;
	int32 mFrameRate = 0;
	double mDeltaTime = 0.0;
	double mGameTime = 0.0;
};

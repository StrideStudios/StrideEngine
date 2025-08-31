#pragma once

#include "core/Common.h"
#include "tracy/Tracy.hpp"
#include "tracy/TracyC.h"

class CProfiler {
public:
	EXPORT static void StartupProfiler();
	EXPORT static void ShutdownProfiler();
};
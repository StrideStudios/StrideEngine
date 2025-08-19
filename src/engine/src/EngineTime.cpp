#include "EngineTime.h"

SEngineTime& SEngineTime::get() {
	static SEngineTime time;
	return time;
}

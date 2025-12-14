#include "basic/core/Singleton.h"

TMap<std::string, TUnique<SObject>>& getSingletons() {
	static TMap<std::string, TUnique<SObject>> gSingletons;
	return gSingletons;
}
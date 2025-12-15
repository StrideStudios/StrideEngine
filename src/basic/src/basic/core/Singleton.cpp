#include "basic/core/Singleton.h"

TMap<std::string, TShared<SObject>>& getSingletons() {
	static TMap<std::string, TShared<SObject>> gSingletons;
	return gSingletons;
}
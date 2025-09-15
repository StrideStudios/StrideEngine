#include "core/Singleton.h"

static std::map<std::string, std::shared_ptr<void>> gSingletons;

std::map<std::string, std::shared_ptr<void>>& getSingletons() {
	return gSingletons;
}
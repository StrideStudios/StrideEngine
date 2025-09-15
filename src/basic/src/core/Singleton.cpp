#include "core/Singleton.h"

std::map<std::string, std::shared_ptr<void>>& getSingletons() {
	static std::map<std::string, std::shared_ptr<void>> gSingletons;
	return gSingletons;
}
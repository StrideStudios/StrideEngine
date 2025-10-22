#include "basic/core/Singleton.h"

std::map<std::string, void*>& getSingletons() {
	static std::map<std::string, void*> gSingletons;
	return gSingletons;
}

bool doesSingletonExist(const std::string& inName) {
	return getSingletons().contains(inName);
}

void* getSingleton(const std::string& inName) {
	return getSingletons().at(inName);
}

void addSingleton(const std::string& inName, void* inValue) {
	getSingletons().insert(std::make_pair(inName, inValue));
}
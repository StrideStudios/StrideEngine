#include "core/Factory.h"

static std::map<std::string, std::unique_ptr<CSharedFactory>> gSharedFactories;

std::unique_ptr<CSharedFactory>& getSharedFactory(const char* inName) {
	if (!gSharedFactories.contains(inName)) {
		gSharedFactories.emplace(inName, std::make_unique<CSharedFactory>());
	}
	return gSharedFactories.at(inName);
}

static std::map<std::string, std::unique_ptr<CStandardFactory>> gStandardFactories;

std::unique_ptr<CStandardFactory>& getStandardFactory(const char* inName) {
	if (!gStandardFactories.contains(inName)) {
		gStandardFactories.emplace(inName, std::make_unique<CStandardFactory>());
	}
	return gStandardFactories.at(inName);
}
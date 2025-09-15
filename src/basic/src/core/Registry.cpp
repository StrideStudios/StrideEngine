#include "core/Registry.h"

static std::map<std::string, std::unique_ptr<CSharedRegistry>> gSharedRegistries;

std::unique_ptr<CSharedRegistry>& getSharedRegistry(const char* inName) {
	if (!gSharedRegistries.contains(inName)) {
		gSharedRegistries.emplace(inName, std::make_unique<CSharedRegistry>());
	}
	return gSharedRegistries.at(inName);
}

static std::map<std::string, std::unique_ptr<CStandardRegistry>> gStandardRegistries;

std::unique_ptr<CStandardRegistry>& getStandardRegistry(const char* inName) {
	if (!gStandardRegistries.contains(inName)) {
		gStandardRegistries.emplace(inName, std::make_unique<CStandardRegistry>());
	}
	return gStandardRegistries.at(inName);
}

static std::unique_ptr<CStaticRegistry> gStaticRegistry;

const std::unique_ptr<CStaticRegistry>& getStaticRegistry() {
	return gStaticRegistry;
}

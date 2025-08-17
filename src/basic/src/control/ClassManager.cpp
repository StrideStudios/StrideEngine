#include "control/ClassManager.h"

static CClassManager gClassManager;

void CClassManager::register_Internal(std::pair<const std::string&, std::function<std::shared_ptr<void>()>> inPair) {
	gClassManager.m_Classes.insert(inPair);
}

std::shared_ptr<void> CClassManager::construct(const std::string& inTypeName) {
	if (!gClassManager.m_Classes.contains(inTypeName)) {
		errs("Could not construct class {}", inTypeName.c_str());
	}
	return gClassManager.m_Classes[inTypeName]();
}
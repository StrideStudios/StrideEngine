#include "control/ClassManager.h"

static CClassManager gClassManager;

void CClassManager::registerClass(const char* typeName, SClass* inClass) {
	std::cout << typeName << " registered." << std::endl;
	gClassManager.m_Classes.insert(std::make_pair(typeName, inClass));
}


std::shared_ptr<void> CClassManager::construct(const char* inTypeName) {
	if (!gClassManager.m_Classes.contains(inTypeName)) {
		errs("Could not construct class {}", inTypeName);
	}
	return gClassManager.m_Classes[inTypeName]->construct();
}
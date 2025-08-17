#include "control/ModuleManager.h"

static CModuleManager gModuleManager;

void CModuleManager::addModule_Internal(std::pair<const std::string&, CModule*> inPair) {
	gModuleManager.m_Modules.insert(inPair);
}

CModule* CModuleManager::getModule_Internal(const std::string& inName) {
	if (!gModuleManager.m_Modules.contains(inName)) {
		errs("Module {} does not exist!", inName.c_str());
	}
	return gModuleManager.m_Modules[inName];
}
#include "basic/control/ResourceManager.h"

CResourceManager& CResourceManager::get() {
	static CResourceManager resourceManager;
	return resourceManager;
}

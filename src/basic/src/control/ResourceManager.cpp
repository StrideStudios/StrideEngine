#include "control/ResourceManager.h"

CResourceManager gResourceManager;

CResourceManager& CResourceManager::get() {
	return gResourceManager;
}
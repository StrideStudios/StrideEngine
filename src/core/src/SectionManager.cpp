#include "SectionManager.h"

CSectionManager& CSectionManager::get() {
	static CSectionManager sectionManager;
	return sectionManager;
}
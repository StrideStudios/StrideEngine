#include "rendercore/Renderer.h"
#include "rendercore/Pass.h"

CPass* CRenderer::getPass(const SClass* cls) const {
	for (const auto pass : m_Passes) {
		if (pass->getClass() == cls) {
			return pass;
		}
	}
	return nullptr;
}

bool CRenderer::hasPass(const SClass* cls) const {
	for (const auto pass : m_Passes) {
		if (pass->getClass() == cls) {
			return true;
		}
	}
	return false;
}

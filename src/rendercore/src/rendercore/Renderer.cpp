#include "rendercore/Renderer.h"
#include "rendercore/Pass.h"

CPass* CRenderer::getPass(const SClass* cls) const {
	for (const auto& pass : m_Passes) {
		if (pass->getClass() == cls) {
			return pass.get();
		}
	}
	return nullptr;
}

bool CRenderer::hasPass(const SClass* cls) const {
	for (const auto& pass : m_Passes) {
		if (pass->getClass() == cls) {
			return true;
		}
	}
	return false;
}

void CRenderer::destroy() {
	for (auto itr = m_Passes.rbegin(); itr != m_Passes.rend(); ++itr) {
		(*itr)->destroy();
	}
}

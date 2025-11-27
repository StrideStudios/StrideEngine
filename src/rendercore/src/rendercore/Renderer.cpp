#include "rendercore/Renderer.h"
#include "rendercore/Pass.h"

static CRenderer* gRenderer;

CRenderer* CRenderer::get() {
	return gRenderer;
}

void CRenderer::set(CRenderer* inRenderer) {
	astsOnce(CRenderer); // Ensure Renderer is set only once
	gRenderer = inRenderer;
	CResourceManager::get().push(gRenderer);
	gRenderer->init();
}

CPass* CRenderer::getPass(const SClass* cls) {
	for (const auto pass : get()->m_Passes) {
		if (pass->getClass() == cls) {
			return pass;
		}
	}
	return nullptr;
}

bool CRenderer::hasPass(const SClass* cls) {
	for (const auto pass : get()->m_Passes) {
		if (pass->getClass() == cls) {
			return true;
		}
	}
	return false;
}

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
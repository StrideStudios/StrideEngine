#include "Renderer.h"

#include "VulkanResourceManager.h"

static CRenderer* gRenderer;

CRenderer* CRenderer::get() {
	return gRenderer;
}

void CRenderer::set(CRenderer* inRenderer, const std::list<CPass*>& inPasses) {
	astsOnce(CRenderer); // Ensure Renderer is set only once
	gRenderer = inRenderer;
	CResourceManager::get().push(gRenderer);
	gRenderer->m_Passes = inPasses;
	gRenderer->init();
}
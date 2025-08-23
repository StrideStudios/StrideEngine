#include "Renderer.h"

#include "VulkanResourceManager.h"

CRenderer* gRenderer;
CVulkanResourceManager gResourceManager;

CRenderer* CRenderer::get() {
	return gRenderer;
}

CVulkanResourceManager& CRenderer::getResourceManager() {
	return gResourceManager;
}

void CRenderer::destroy() {
	gResourceManager.flush();
}

void CRenderer::set(CRenderer* inRenderer) {
	astsOnce(CRenderer); // Ensure Renderer is set only once
	gRenderer = inRenderer;
}
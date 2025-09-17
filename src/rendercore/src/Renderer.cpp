#include "Renderer.h"

#include "VkBootstrap.h"

static CRenderer* gRenderer;

CRenderer* CRenderer::get() {
	return gRenderer;
}

const VkInstance& CRenderer::vkInstance() {
	return instance().instance;
}

const VkPhysicalDevice& CRenderer::vkPhysicalDevice() {
	return physicalDevice().physical_device;
}

const VkDevice& CRenderer::vkDevice() {
	return device().device;
}

void CRenderer::set(CRenderer* inRenderer) {
	astsOnce(CRenderer); // Ensure Renderer is set only once
	gRenderer = inRenderer;
	CResourceManager::get().push(gRenderer);
	gRenderer->init();
}
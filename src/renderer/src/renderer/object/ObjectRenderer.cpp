#include "renderer/object/ObjectRenderer.h"

static CRendererManager gRendererManager;

CRendererManager& CRendererManager::get() {
	return gRendererManager;
}

void CRendererManager::registerRenderer(const char* inPassName, const std::string& inRendererName, std::shared_ptr<CObjectRenderer> inRenderer) {
	/*auto renderer = std::make_pair(inRendererName, inRenderer);
	if (gRendererManager.m_Renderers.empty() || !gRendererManager.m_Renderers.contains(inPassName)) {
		std::map<std::string, std::shared_ptr<CObjectRenderer>> map;
		map.insert(renderer);
		gRendererManager.m_Renderers.insert(std::make_pair(inPassName, map));
		return;
	}
	auto& pass = gRendererManager.m_Renderers.at(inPassName);
	pass.insert(renderer);*/
}

void CRendererManager::startPass(const std::string& inPass) {
	for (auto& pair : get().m_Renderers[inPass]) {
		pair.second->begin();
	}
}

void CRendererManager::endPass(const std::string& inPass) {
	for (auto& pair : get().m_Renderers[inPass]) {
		pair.second->end();
	}
}


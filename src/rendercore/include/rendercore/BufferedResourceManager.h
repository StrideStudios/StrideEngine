#pragma once

#include "basic/control/ResourceManager.h"
#include "basic/core/Common.h"
#include "basic/core/Object.h"

#include "rendercore/Renderer.h"

class CBufferedResourceManager : public SBase, public IInitializable, public IDestroyable {

	MAKE_LAZY_SINGLETON(CBufferedResourceManager)

public:

	virtual void init() override {
		m_Managers.resize(CRenderer::get()->getBufferingType().getFrameOverlap());
	}

	virtual void destroy() override {
		for (auto& manager : m_Managers) {
			manager.flush();
		}
	}

	force_inline CResourceManager& getCurrentResourceManager() {
		return m_Managers[CRenderer::get()->getBufferingType().getFrameIndex()];
	}

	force_inline const CResourceManager& getCurrentResourceManager() const {
		return m_Managers[CRenderer::get()->getBufferingType().getFrameIndex()];
	}

private:

	std::vector<CResourceManager> m_Managers;

};

#pragma once

#include <memory>

#include "rendercore/Material.h"
#include "scene/base/RenderableObject.h"

class CSprite : public CRenderableViewportObject {

	REGISTER_CLASS(CSprite, CRenderableViewportObject)

public:

	// Surface Data
	CMaterial* material;
};

class CInstancedSprite : public CSprite {

	REGISTER_CLASS(CInstancedSprite, CSprite)

public:

	CInstancedSprite() {
		m_Instancer.flush();
	}

	virtual IInstancer& getInstancer() override {
		return m_Instancer;
	}

	virtual size_t addInstance(const Transform2f& inTransform) {
		return m_Instancer.addInstance(SInstance{inTransform.toMatrix()});
	}

	virtual void setInstance(const size_t inInstanceIndex, const Transform2f& inTransform) {
		SInstance& instance = m_Instancer.getInstance(inInstanceIndex);
		instance.Transform = inTransform.toMatrix();
		m_Instancer.setDirty();
	}

	virtual void removeInstance(const size_t instance) {
		m_Instancer.removeInstance(instance);
	}

	virtual CArchive& save(CArchive& inArchive) const override {
		CSprite::save(inArchive);
		inArchive << m_Instancer;
		return inArchive;
	}

	virtual CArchive& load(CArchive& inArchive) override {
		CSprite::load(inArchive);
		inArchive >> m_Instancer;
		return inArchive;
	}

private:

	SDynamicInstancer m_Instancer;

};
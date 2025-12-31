#pragma once

#include <memory>

#include "rendercore/Material.h"
#include "scene/base/RenderableObject.h"

class CSprite : public CRenderableViewportObject {

	REGISTER_CLASS(CSprite, CRenderableViewportObject)

public:

	virtual IInstancer& getInstancer() override {
		return m_Instancer;
	}

	virtual CMaterial* getMaterial() override {
		return material;
	}

	// Surface Data
	CMaterial* material;

	SSingleInstancer m_Instancer;

};

class CInstancedSprite : public CRenderableViewportObject {

	REGISTER_CLASS(CInstancedSprite, CSprite)

public:

	CInstancedSprite() {
		m_Instancer.flush();
	}

	virtual IInstancer& getInstancer() override {
		return m_Instancer;
	}

	virtual CMaterial* getMaterial() override {
		return material;
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
		CViewportObject::save(inArchive);
		inArchive << m_Instancer;
		return inArchive;
	}

	virtual CArchive& load(CArchive& inArchive) override {
		CViewportObject::load(inArchive);
		inArchive >> m_Instancer;
		return inArchive;
	}

	// Surface Data
	CMaterial* material = nullptr;

private:

	SDynamicInstancer m_Instancer;

};
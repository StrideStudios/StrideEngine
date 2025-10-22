#pragma once

#include <memory>

#include "rendercore/Material.h"
#include "scene/base/RenderableObject.h"

class CSprite : public CRenderableViewportObject {

	REGISTER_CLASS(CSprite, CRenderableViewportObject)

public:

	// Surface Data
	std::shared_ptr<CMaterial> material;
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

	virtual uint32 addInstance(const Transform2f& inPosition) {
		return m_Instancer.push(SInstance{inPosition.toMatrix()});
	}

	virtual void setInstance(const int32 inInstanceIndex, const Transform2f& inPosition) {
		SInstance& instance = m_Instancer.instances[inInstanceIndex];
		instance.Transform = inPosition.toMatrix();
		m_Instancer.setDirty();
	}

	virtual void removeInstance(const uint32 instance) {
		m_Instancer.remove(instance);
	}

	virtual CArchive& save(CArchive& inArchive) override {
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

	SInstancer m_Instancer;

};
#pragma once

#include <memory>

#include "Common.h"
#include "Widget.h"
#include "EngineLoader.h"

class CSprite : public CWidget {

public:

	struct Data {
		Vector2f mUV0;
		Vector2f mUV1;
		Vector4f mColor;
	};

	// Surface Data
	std::shared_ptr<CMaterial> material;
};

class CInstancedSprite : public CSprite {

public:

	CInstancedSprite() {
		m_Instancer.flush();
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

	virtual void onPositionChanged() override {
		m_Instancer.setDirty();
	}

	virtual SInstancer& getInstancer() override {
		return m_Instancer;
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

};
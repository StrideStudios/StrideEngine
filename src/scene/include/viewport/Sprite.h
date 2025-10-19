#pragma once

#include <memory>

#include "Material.h"
#include "base/RenderableObject.h"

class CSprite : public CRenderableViewportObject {

public:

	// Surface Data
	std::shared_ptr<CMaterial> material;
};

class CInstancedSprite : public CSprite {

public:

	SInstancer instancer;

	CInstancedSprite() {
		instancer.flush();
	}

	virtual IInstancer& getInstancer() override {
		return instancer;
	}

	virtual uint32 addInstance(const Transform2f& inPosition) {
		return instancer.push(SInstance{inPosition.toMatrix()});
	}

	virtual void setInstance(const int32 inInstanceIndex, const Transform2f& inPosition) {
		SInstance& instance = instancer.instances[inInstanceIndex];
		instance.Transform = inPosition.toMatrix();
		instancer.setDirty();
	}

	virtual void removeInstance(const uint32 instance) {
		instancer.remove(instance);
	}

	virtual CArchive& save(CArchive& inArchive) override {
		CSprite::save(inArchive);
		inArchive << instancer;
		return inArchive;
	}

	virtual CArchive& load(CArchive& inArchive) override {
		CSprite::load(inArchive);
		inArchive >> instancer;
		return inArchive;
	}

};
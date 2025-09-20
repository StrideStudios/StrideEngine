#pragma once

#include <memory>

#include "Material.h"
#include "base/RenderableObject.h"

class CSprite : public CRenderableViewportObject {

public:

	virtual const char* getRendererType() override {
		return "TODO";
	}

	// Surface Data
	std::shared_ptr<CMaterial> material;
};

class CInstancedSprite : public CSprite {

public:

	CInstancedSprite() {
		CInstancedSprite::getInstancer().flush();
	}

	virtual uint32 addInstance(const Transform2f& inPosition) {
		return getInstancer().push(SInstance{inPosition.toMatrix()});
	}

	virtual void setInstance(const int32 inInstanceIndex, const Transform2f& inPosition) {
		SInstance& instance = getInstancer().instances[inInstanceIndex];
		instance.Transform = inPosition.toMatrix();
		getInstancer().setDirty();
	}

	virtual void removeInstance(const uint32 instance) {
		getInstancer().remove(instance);
	}

	virtual CArchive& save(CArchive& inArchive) override {
		CSprite::save(inArchive);
		inArchive << getInstancer();
		return inArchive;
	}

	virtual CArchive& load(CArchive& inArchive) override {
		CSprite::load(inArchive);
		inArchive >> getInstancer();
		return inArchive;
	}

};
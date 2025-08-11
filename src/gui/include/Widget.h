#pragma once

#include <glm/ext/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include "Archive.h"
#include "Common.h"
#include "Engine.h"
#include "Viewport.h"

class CWidget : ISerializable {

	REGISTER_CLASS(CWidget)

public:

	virtual ~CWidget() = default;

	virtual CArchive& save(CArchive& inArchive) override {
		inArchive << mName;
		inArchive << mOrigin;
		inArchive << mPosition;
		inArchive << mRotation;
		inArchive << mScale;
		return inArchive;
	}

	virtual CArchive& load(CArchive& inArchive) override {
		inArchive >> mName;
		inArchive >> mOrigin;
		inArchive >> mPosition;
		inArchive >> mRotation;
		inArchive >> mScale;
		return inArchive;
	}

	no_discard virtual Matrix4f getTransformMatrix() const {
		const Vector2f screenSize = CEngine::get().getViewport().mExtent;
		const Vector2f screenScale = mScale * screenSize;
		const Vector2f screenPos = mPosition * screenSize - mOrigin * screenScale;

		Matrix4f mat = glm::translate(Matrix4f(1.0), Vector3f(screenPos, 0.f));
		mat *= glm::mat4_cast(glm::qua(Vector3f(mRotation, 0.f, 0.f)));
		mat = glm::scale(mat, Vector3f(screenScale, 1.f));
		return mat;
	}

	std::string mName{"Widget"};

	Vector2f mOrigin{0.f};
	Vector2f mPosition{0.f};
	float mRotation = 0.f;
	Vector2f mScale{1.f};
};

#pragma once

#include <glm/ext/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include "Archive.h"
#include "Common.h"

class IRenderable {
public:
	virtual std::shared_ptr<struct SStaticMesh> getMesh() = 0;

	virtual Matrix4f getTransformMatrix() = 0;
};

class CSceneObject : public ISerializable {

public:
	virtual ~CSceneObject() = default;

	no_discard Vector3d getPosition() const {
		return mPosition;
	}

	no_discard Vector3f getRotation() const {
		return mRotation;
	}

	no_discard Vector3f getScale() const {
		return mScale;
	}

	void setPosition(const Vector3d& inPosition) {
		mPosition = inPosition;
	}

	void setRotation(const Vector3f& inRotation) {
		mRotation = inRotation;
	}

	void setScale(const Vector3f& inScale) {
		mScale = inScale;
	}

	CArchive& save(CArchive& inArchive) override {
		inArchive << mPosition;
		inArchive << mRotation;
		inArchive << mScale;
		return inArchive;
	}

	CArchive& load(CArchive& inArchive) override {
		inArchive >> mPosition;
		inArchive >> mRotation;
		inArchive >> mScale;
		return inArchive;
	}

protected:

	Vector3d mPosition{0, 0, 0};

	//TODO: always store as quaternion internally, and just convert so user doesnt have to use it
	Vector3f mRotation{0,0, 0};

	Vector3f mScale{1,1, 1};

};

class CRenderableObject : public CSceneObject, public IRenderable {

	Matrix4f getTransformMatrix() override {
		Matrix4f mat = glm::translate(Matrix4f(1.0), (Vector3f)getPosition());
		mat *= glm::mat4_cast(glm::qua(mRotation));
		mat = glm::scale(mat, mScale);
		return mat;
	}
};
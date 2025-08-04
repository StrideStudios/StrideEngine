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

class CSceneObject {

public:

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

protected:

	Vector3d mPosition{0, 0, 0};

	//TODO: always store as quaternion internally, and just convert so user doesnt have to use it
	Vector3f mRotation{0,0, 0};

	Vector3f mScale{1,1, 1};

private:

	friend CArchive& operator<<(CArchive& inArchive, const CSceneObject& inObject) {
		inArchive << inObject.mPosition;
		inArchive << inObject.mRotation;
		inArchive << inObject.mScale;
		return inArchive;
	}

	friend CArchive& operator>>(CArchive& inArchive, CSceneObject& inObject) {
		inArchive >> inObject.mPosition;
		inArchive >> inObject.mRotation;
		inArchive >> inObject.mScale;
		return inArchive;
	}
};

class CRenderableObject : public CSceneObject, public IRenderable {

	Matrix4f getTransformMatrix() override {
		Matrix4f mat = glm::translate(Matrix4f(1.0), (Vector3f)getPosition());
		mat *= glm::mat4_cast(glm::qua(mRotation));
		mat = glm::scale(mat, mScale);
		return mat;
	}
};
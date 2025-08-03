#pragma once

#include "Archive.h"
#include "Common.h"

class CSceneObject {

public:

	Vector3f getPosition() const {
		return mPosition;
	}

	Vector3f getRotation() const {
		return mRotation;
	}

	void setPosition(const Vector3f inPosition) {
		mPosition = inPosition;
	}

	void setRotation(const Vector3f inRotation) {
		mRotation = inRotation;
	}

protected:

	Vector3f mPosition{0, 0, 0};

	Vector3f mRotation{0,0, 0};

private:

	friend CArchive& operator<<(CArchive& inArchive, const CSceneObject& inObject) {
		inArchive << inObject.mPosition;
		inArchive << inObject.mRotation;
		return inArchive;
	}

	friend CArchive& operator>>(CArchive& inArchive, CSceneObject& inObject) {
		inArchive >> inObject.mPosition;
		inArchive >> inObject.mRotation;
		return inArchive;
	}
};
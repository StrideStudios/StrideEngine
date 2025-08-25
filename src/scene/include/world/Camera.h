#pragma once

#include "base/SceneObject.h"

class EXPORT CCamera : public CWorldObject, public IDestroyable {

	REGISTER_CLASS(CCamera)

public:

	CCamera();

	virtual Matrix4f getRotationMatrix();

	virtual Matrix4f getViewProjectionMatrix();

	void update();

	bool mShowMouse = true;

	//TODO: base class for velocity (movable object)
	Vector3f mVelocity{0,0,0};

	float mFOV;
};
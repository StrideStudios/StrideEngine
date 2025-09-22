#pragma once

#include "base/SceneObject.h"

class CCamera : public CWorldObject, public IDestroyable {

	REGISTER_CLASS(CCamera, CWorldObject)

public:

	EXPORT CCamera();

	EXPORT virtual Matrix4f getRotationMatrix();

	EXPORT virtual Matrix4f getViewProjectionMatrix();

	EXPORT void update();

	bool mShowMouse = true;

	//TODO: base class for velocity (movable object)
	Vector3f mVelocity{0,0,0};

	float mFOV;
};
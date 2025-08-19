#include "Camera.h"

#include <glm/gtx/quaternion.hpp>

#include "Engine.h"
#include "EngineSettings.h"
#include "EngineTime.h"
#include "Input.h"
#include "Viewport.h"

#define SETTINGS_CATEGORY "Camera"
ADD_COMMAND(float, FieldOfView, 70.f, 0.f, 180.f);
ADD_COMMAND(float, Sensitivity, 1.0f, 0.f, 2.f);
ADD_COMMAND(float, CameraSpeed, 200.f, 0.f, 1000.f);
#undef SETTINGS_CATEGORY

CCamera::CCamera(): mFOV(FieldOfView.get()) {}

Matrix4f CCamera::getRotationMatrix() {
	glm::quat pitchRotation = glm::angleAxis(getRotation().y, Vector3f{ 1.f, 0.f, 0.f });
	glm::quat yawRotation = glm::angleAxis(getRotation().x, Vector3f{ 0.f, -1.f, 0.f });

	return glm::toMat4(yawRotation) * glm::toMat4(pitchRotation);
}

Matrix4f CCamera::getViewProjectionMatrix() {
	const Matrix4f cameraTranslation = glm::translate(Matrix4f(1.f), glm::vec3(getPosition()));
	Matrix4f viewMatrix = glm::inverse(cameraTranslation * getRotationMatrix());

	// camera projection
	const Extent32u extent = CEngineViewport::get().mExtent;
	const float tanHalfFov = tan(glm::radians(mFOV) / 2.f);
	const float aspect = (float)extent.x / (float)extent.y;

	Matrix4f projection(0.f);
	projection[0][0] = 1.f / (aspect * tanHalfFov);
	projection[1][1] = 1.f / tanHalfFov;
	projection[2][3] = - 1.f;
	projection[3][2] = 0.1f;

	// invert the Y direction on projection matrix so that we are more similar
	// to opengl and gltf axis
	projection[1][1] *= -1;

	return projection * viewMatrix;
}

void CCamera::update() {
	// Set FOV
	mFOV = FieldOfView.get();

	// Engine camera moves only when right clicking
	mShowMouse = !CInput::getMousePressed(3); //RightClick

	if (!mShowMouse) {
		Vector3f rotation = getRotation();
		rotation.x += Sensitivity.get() * (CInput::getMouseVelocity().x / 360.f);// * (float)SEngineTime::get().mDeltaTime;
		rotation.y -= Sensitivity.get() * (CInput::getMouseVelocity().y / 360.f);// * (float)SEngineTime::get().mDeltaTime;
		setRotation(rotation);
	}

	float forwardAxis = CInput::getKeyPressed(EKey::S) ? 0.25f : CInput::getKeyPressed(EKey::W) ? -0.25f : 0.f;
	float rightAxis = CInput::getKeyPressed(EKey::D) ? 0.25f : CInput::getKeyPressed(EKey::A) ? -0.25f : 0.f;
	float upAxis = CInput::getKeyPressed(EKey::E) ? 0.25f : CInput::getKeyPressed(EKey::Q) ? -0.25f : 0.f;

	mVelocity = {rightAxis, upAxis, forwardAxis};

	Vector2f movement{mVelocity.x * 0.5f, mVelocity.z * 0.5f};
	Vector3f position = getPosition();
	position += Vector3f(getRotationMatrix() * Vector4f(movement.x, 0.f, movement.y, 0.f)) * (float)SEngineTime::get().mDeltaTime * CameraSpeed.get();
	position += Vector3f(0.f, mVelocity.y * 0.5f, 0.f) * (float)SEngineTime::get().mDeltaTime * CameraSpeed.get();
	setPosition(position);
}

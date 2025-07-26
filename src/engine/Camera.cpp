#include "Camera.h"

//TODO: why this?
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>

#include "EngineSettings.h"

#define COMMAND_CATEGORY "Camera"
ADD_COMMAND(float, FieldOfView, 70.f, 0.f, 180.f);
#undef COMMAND_CATEGORY

CCamera::CCamera(): mFOV(FieldOfView.get()) {

}

Matrix4f CCamera::getViewMatrix() const {
	const Matrix4f cameraTranslation = glm::translate(Matrix4f(1.f), glm::vec3(mPosition));
	const Matrix4f cameraRotation = getRotationMatrix();
	return glm::inverse(cameraTranslation * cameraRotation);
}

Matrix4f CCamera::getRotationMatrix() const {
	glm::quat pitchRotation = glm::angleAxis(mRotation.y, Vector3f{ 1.f, 0.f, 0.f });
	glm::quat yawRotation = glm::angleAxis(mRotation.x, Vector3f{ 0.f, -1.f, 0.f });

	return glm::toMat4(yawRotation) * glm::toMat4(pitchRotation);
}

void CCamera::processSDLEvent(const SDL_Event &e) {

	switch (e.type) {
		case SDL_EVENT_KEY_DOWN:
			keyMap[static_cast<EKey>(e.key.key)] = true;
			break;
		case SDL_EVENT_KEY_UP:
			keyMap[static_cast<EKey>(e.key.key)] = false;
			break;
		default:
			break;
	}

	switch (e.type) {
		case SDL_EVENT_KEY_DOWN:
			switch (e.key.key) {
			case M:
					bShowMouse = !bShowMouse;
					break;
			default:
					break;
			}
			break;
		case SDL_EVENT_MOUSE_BUTTON_DOWN:
			switch (e.button.button) {
				case 1:
					mLeftMouseDown = true;
					break;
				case 3:
					mRightMouseDown = true;
					bShowMouse = false;
					break;
				default:
					break;
			}
			break;
		case SDL_EVENT_MOUSE_BUTTON_UP:
			switch (e.button.button) {
				case 1:
					mLeftMouseDown = false;
					break;
				case 3:
					mRightMouseDown = false;
					bShowMouse = true;
					break;
				default:
					break;
			}
			break;
		case SDL_EVENT_MOUSE_MOTION:
			if (!bShowMouse) {
				mRotation.x += e.motion.xrel / 250.f;
				mRotation.y -= e.motion.yrel / 250.f;
			}
			break;
	}
}

void CCamera::update() {

	float forwardAxis = keyMap[S] ? 0.25f : keyMap[W] ? -0.25f : 0.f;
	float rightAxis = keyMap[D] ? 0.25f : keyMap[A] ? -0.25f : 0.f;
	float upAxis = keyMap[E] ? 0.25f : keyMap[Q] ? -0.25f : 0.f;

	mVelocity = {rightAxis, upAxis, forwardAxis};

	mFOV = FieldOfView.get();
	Matrix4f cameraRotation = getRotationMatrix();
	Vector2f movement{mVelocity.x * 0.5f, mVelocity.z * 0.5f};
	mPosition += Vector3f(cameraRotation * Vector4f(movement.x, 0.f, movement.y, 0.f));
	mPosition += Vector3f(0.f, mVelocity.y * 0.5f, 0.f);
}

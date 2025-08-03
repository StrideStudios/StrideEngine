#include "Camera.h"

//TODO: why this?
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>

#include "Engine.h"
#include "EngineSettings.h"
#include "EngineTextures.h"
#include "VulkanRenderer.h"

#define COMMAND_CATEGORY "Camera"
ADD_COMMAND(float, FieldOfView, 70.f, 0.f, 180.f);
#undef COMMAND_CATEGORY

CCamera::CCamera(): mFOV(FieldOfView.get()) {
	mouseArray.fill(false);
}

void CCamera::processSDLEvent(const SDL_Event &e) {

	switch (e.type) {
		case SDL_EVENT_KEY_DOWN:
			keyMap[static_cast<EKey>(e.key.key)] = true;
			break;
		case SDL_EVENT_KEY_UP:
			keyMap[static_cast<EKey>(e.key.key)] = false;
			break;
		case SDL_EVENT_MOUSE_BUTTON_DOWN:
			mouseArray[e.button.button] = true;
			break;
		case SDL_EVENT_MOUSE_BUTTON_UP:
			mouseArray[e.button.button] = false;
			break;
		case SDL_EVENT_MOUSE_MOTION:
			if (!mShowMouse) {
				mRotation.x += e.motion.xrel / 250.f;
				mRotation.y -= e.motion.yrel / 250.f;
			}
			break;
		default:
			break;
	}

	mShowMouse = !mouseArray[3]; //RightClick
}

void CCamera::update() {

	// Update View matrices
	{
		glm::quat pitchRotation = glm::angleAxis(mRotation.y, Vector3f{ 1.f, 0.f, 0.f });
		glm::quat yawRotation = glm::angleAxis(mRotation.x, Vector3f{ 0.f, -1.f, 0.f });

		mRotationMatrix = glm::toMat4(yawRotation) * glm::toMat4(pitchRotation);

		const Matrix4f cameraTranslation = glm::translate(Matrix4f(1.f), glm::vec3(mPosition));
		const Matrix4f cameraRotation = mRotationMatrix;
		mViewMatrix = glm::inverse(cameraTranslation * cameraRotation);

		// camera projection
		const auto [x, y, z] = CEngine::renderer().mEngineTextures->mDrawImage->mImageExtent;
		const float tanHalfFov = tan(glm::radians(mFOV) / 2.f);
		const float aspect = (float)x / (float)y;

		Matrix4f projection(0.f);
		projection[0][0] = 1.f / (aspect * tanHalfFov);
		projection[1][1] = 1.f / tanHalfFov;
		projection[2][3] = - 1.f;
		projection[3][2] = 0.1f;

		// invert the Y direction on projection matrix so that we are more similar
		// to opengl and gltf axis
		projection[1][1] *= -1;

		mProjectionMatrix = projection;

		mViewProjectionMatrix = mProjectionMatrix * mViewMatrix;

		Vector2f size = {x, y};
		//const Matrix4f orthogonalProjection = glm::ortho(-size.x / 2.f, size.x / 2.f, size.y / 2.f, -size.y / 2.f, 0.f, 1.f);
		const Matrix4f orthogonalProjection = glm::ortho(0.f, size.x, 0.f, size.y, 0.f, 1.f);
		Matrix4f view = glm::translate(glm::mat4{1.f}, Vector3f(100.f, 50.f, 0.f));
		view = glm::scale(view, Vector3f(x, y, 1.f)); //TODO: 2d camera pos
		mOrthogonalViewProjectionMatrix = orthogonalProjection;
	}

	float forwardAxis = keyMap[S] ? 0.25f : keyMap[W] ? -0.25f : 0.f;
	float rightAxis = keyMap[D] ? 0.25f : keyMap[A] ? -0.25f : 0.f;
	float upAxis = keyMap[E] ? 0.25f : keyMap[Q] ? -0.25f : 0.f;

	mVelocity = {rightAxis, upAxis, forwardAxis};

	mFOV = FieldOfView.get();
	Vector2f movement{mVelocity.x * 0.5f, mVelocity.z * 0.5f};
	mPosition += Vector3f(mRotationMatrix * Vector4f(movement.x, 0.f, movement.y, 0.f));
	mPosition += Vector3f(0.f, mVelocity.y * 0.5f, 0.f);
}

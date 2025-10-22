#include "scene/base/SceneObject.h"

#include "engine/Viewport.h"

Matrix4f CViewportObject::getTransformMatrix() const {
	const Vector2f screenSize = CEngineViewport::get().mExtent;
	return glm::scale(m_Transform.toMatrix(), Vector3f{screenSize, 1.f});
}

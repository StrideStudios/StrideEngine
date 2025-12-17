#include "scene/base/SceneObject.h"

#include "engine/Engine.h"
#include "engine/Viewport.h"

Matrix4f CViewportObject::getTransformMatrix() const {
	const Vector2f screenSize = CEngine::get().getViewport()->mExtent;
	return glm::scale(m_Transform.toMatrix(), Vector3f{screenSize, 1.f});
}

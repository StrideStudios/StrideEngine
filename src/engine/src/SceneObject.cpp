#include "SceneObject.h"

#include "Engine.h"
#include "Viewport.h"

Matrix4f CSceneObject2D::getTransformMatrix() const  {
	const Vector2f screenSize = CEngine::get().getViewport().mExtent;
	const Vector2f screenScale = m_Scale * screenSize;
	const Vector2f screenPos = m_Position * screenSize - m_Origin * screenScale;

	Matrix4f mat = glm::translate(Matrix4f(1.0), Vector3f(screenPos, 0.f));
	mat *= glm::mat4_cast(glm::qua(Vector3f(m_Rotation, 0.f, 0.f)));
	mat = glm::scale(mat, Vector3f(screenScale, 1.f));
	return mat;
}
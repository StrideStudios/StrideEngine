#pragma once

#include <glm/ext/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include "SceneObject.h"
#include "Viewport.h"

class CWidget : ISerializable {

	REGISTER_CLASS(CWidget)

public:

	virtual ~CWidget() = default;

	virtual CArchive& save(CArchive& inArchive) override {
		inArchive << mName;
		inArchive << m_Origin;
		inArchive << m_Position;
		inArchive << m_Rotation;
		inArchive << m_Scale;
		return inArchive;
	}

	virtual CArchive& load(CArchive& inArchive) override {
		inArchive >> mName;
		inArchive >> m_Origin;
		inArchive >> m_Position;
		inArchive >> m_Rotation;
		inArchive >> m_Scale;
		return inArchive;
	}

	SInstancer m_Instancer{1}; //TODO: bad, shouldnt have an instancer in a non-instanced widget

	virtual SInstancer& getInstancer() {
		return m_Instancer;
	}

	no_discard virtual Matrix4f getTransformMatrix() const {
		const Vector2f screenSize = CEngineViewport::get().mExtent;
		const Vector2f screenScale = m_Scale * screenSize;
		const Vector2f screenPos = m_Position * screenSize - m_Origin * screenScale;

		Matrix4f mat = glm::translate(Matrix4f(1.0), Vector3f(screenPos, 0.f));
		mat *= glm::mat4_cast(glm::qua(Vector3f(m_Rotation, 0.f, 0.f)));
		mat = glm::scale(mat, Vector3f(screenScale, 1.f));
		return mat;
	}

	std::string mName{"Widget"};

	Vector2f getOrigin() const { return m_Origin; }
	Vector2f getPosition() const { return m_Position; }
	float getRotation() const { return m_Rotation; }
	Vector2f getScale() const { return m_Scale; }

	virtual void onPositionChanged() {
		m_Instancer.setDirty();
	}

	void getOrigin(const Vector2f inOrigin) {
		m_Origin = inOrigin;
		onPositionChanged();
	}

	void setPosition(const Vector2f inPosition) {
		m_Position = inPosition;
		onPositionChanged();
	}

	void setRotation(const float inRotation) {
		m_Rotation = inRotation;
		onPositionChanged();
	}

	void setScale(const Vector2f inScale) {
		m_Scale = inScale;
		onPositionChanged();
	}

private:

	Vector2f m_Origin{0.f};
	Vector2f m_Position{0.f};
	float m_Rotation = 0.f;
	Vector2f m_Scale{1.f};
};

#pragma once

#include <glm/ext/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include "Instancer.h"
#include "VulkanResources.h"

class CSceneObject : public ISerializable {

	//REGISTER_CLASS(CSceneObject)

public:
	virtual ~CSceneObject() = default;

	CSceneObject() = default;

	CSceneObject(const std::string& inName): mName(inName) {}

	no_discard virtual Matrix4f getTransformMatrix() const = 0;

	virtual CArchive& save(CArchive& inArchive) override {
		inArchive << mName;
		return inArchive;
	}

	virtual CArchive& load(CArchive& inArchive) override {
		inArchive >> mName;
		return inArchive;
	}

	virtual void onMoved() {}

	std::string mName{"Scene Object"};

};

class EXPORT CViewportObject : public CSceneObject {

	REGISTER_CLASS(CViewportObject)

public:

	CViewportObject(): CSceneObject("Viewport Object") {}

	no_discard virtual Matrix4f getTransformMatrix() const override;

	virtual CArchive& save(CArchive& inArchive) override {
		CSceneObject::save(inArchive);
		inArchive << m_Transform;
		return inArchive;
	}

	virtual CArchive& load(CArchive& inArchive) override {
		CSceneObject::load(inArchive);
		inArchive >> m_Transform;
		return inArchive;
	}

	Vector2f getOrigin() const { return m_Transform.getOrigin(); }
	Vector2f getPosition() const { return m_Transform.getPosition(); }
	float getRotation() const { return m_Transform.getRotation(); }
	Vector2f getScale() const { return m_Transform.getScale(); }

	void setOrigin(Vector2f inOrigin) {
		m_Transform.setOrigin(inOrigin);
		onMoved();
	}

	void setPosition(Vector2f inPosition) {
		m_Transform.setPosition(inPosition);
		onMoved();
	}

	void setRotation(const float inRotation) {
		m_Transform.setRotation(inRotation);
		onMoved();
	}

	//TODO: 2d viewport scale should be based on relative coordinates
	void setScale(Vector2f inScale) {
		//const Vector2f screenSize = CEngineViewport::get().mExtent;
		//m_Transform.setScale(inScale * screenSize);
		m_Transform.setScale(inScale);
		onMoved();
	}

private:

	Transform2f m_Transform;
};

class EXPORT CWorldObject : public CSceneObject {

	REGISTER_CLASS(CWorldObject)

public:

	CWorldObject(): CSceneObject("World Object") {}

	no_discard virtual Matrix4f getTransformMatrix() const override {
		return m_Transform.toMatrix();
	}

	virtual CArchive& save(CArchive& inArchive) override {
		CSceneObject::save(inArchive);
		inArchive << m_Transform;
		return inArchive;
	}

	virtual CArchive& load(CArchive& inArchive) override {
		CSceneObject::load(inArchive);
		inArchive >> m_Transform;
		return inArchive;
	}

	Vector3f getPosition() const { return m_Transform.getPosition(); }
	Vector3f getRotation() const { return m_Transform.getRotation(); }
	Vector3f getScale() const { return m_Transform.getScale(); }

	void setPosition(const Vector3f inPosition) {
		m_Transform.setPosition(inPosition);
		onMoved();
	}

	void setRotation(const Vector3f inRotation) {
		m_Transform.setRotation(inRotation);
		onMoved();
	}

	void setScale(const Vector3f inScale) {
		m_Transform.setScale(inScale);
		onMoved();
	}

private:

	Transform3f m_Transform;

};
#pragma once

#include "Instancer.h"
#include "core/Object.h"
#include "core/Class.h"

#define REGISTER_SCENE_OBJECT(n) \
	ADD_TO_FACTORY(SObjectFactory, n) \
	REGISTER_CLASS(n)

class CSceneObject : public SObject, public ISerializable {
public:
	DEFINE_CLASS(CSceneObject, SObject)

	REGISTER_CLASS(CSceneObject)

public:

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

class CViewportObject : public CSceneObject {

	REGISTER_SCENE_OBJECT(CViewportObject)

public:

	CViewportObject(): CSceneObject("Viewport Object") {}

	no_discard EXPORT virtual Matrix4f getTransformMatrix() const override;

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

class CWorldObject : public CSceneObject {
public:

	DEFINE_CLASS(CWorldObject, CSceneObject)

	REGISTER_SCENE_OBJECT(CWorldObject)

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
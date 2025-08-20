#pragma once

#include <glm/ext/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

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

	SInstancer m_Instancer{1}; //TODO: bad, shouldnt have an instancer in a non-instanced widget

	virtual SInstancer& getInstancer() {
		return m_Instancer;
	}

	bool mIsDirty = true;

	bool isDirty() const {
		return mIsDirty;
	}

	void setDirty() {
		mIsDirty = true;
		m_Instancer.setDirty();
	}

	std::string mName{"Scene Object"};

};

class EXPORT CSceneObject2D : public CSceneObject {

	REGISTER_CLASS(CSceneObject2D)

public:

	CSceneObject2D(): CSceneObject("Widget") {}

	no_discard virtual Matrix4f getTransformMatrix() const override;

	virtual CArchive& save(CArchive& inArchive) override {
		CSceneObject::save(inArchive);
		inArchive << m_Origin;
		inArchive << m_Position;
		inArchive << m_Rotation;
		inArchive << m_Scale;
		return inArchive;
	}

	virtual CArchive& load(CArchive& inArchive) override {
		CSceneObject::load(inArchive);
		inArchive >> m_Origin;
		inArchive >> m_Position;
		inArchive >> m_Rotation;
		inArchive >> m_Scale;
		return inArchive;
	}

	Vector2f getOrigin() const { return m_Origin; }
	Vector2f getPosition() const { return m_Position; }
	float getRotation() const { return m_Rotation; }
	Vector2f getScale() const { return m_Scale; }

	void getOrigin(const Vector2f inOrigin) {
		m_Origin = inOrigin;
		setDirty();
	}

	void setPosition(const Vector2f inPosition) {
		m_Position = inPosition;
		setDirty();
	}

	void setRotation(const float inRotation) {
		m_Rotation = inRotation;
		setDirty();
	}

	void setScale(const Vector2f inScale) {
		m_Scale = inScale;
		setDirty();
	}

private:

	Vector2f m_Origin{0.f};
	Vector2f m_Position{0.f};
	float m_Rotation = 0.f;
	Vector2f m_Scale{1.f};
};

class EXPORT CSceneObject3D : public CSceneObject {

	REGISTER_CLASS(CSceneObject3D)

public:

	CSceneObject3D(): CSceneObject("Object") {}

	no_discard virtual Matrix4f getTransformMatrix() const override {
		Matrix4f mat = glm::translate(Matrix4f(1.0), m_Position);
		mat *= glm::mat4_cast(glm::qua(m_Rotation));
		mat = glm::scale(mat, m_Scale);
		return mat;
	}

	virtual CArchive& save(CArchive& inArchive) override {
		CSceneObject::save(inArchive);
		inArchive << m_Position;
		inArchive << m_Rotation;
		inArchive << m_Scale;
		return inArchive;
	}

	virtual CArchive& load(CArchive& inArchive) override {
		CSceneObject::load(inArchive);
		inArchive >> m_Position;
		inArchive >> m_Rotation;
		inArchive >> m_Scale;
		return inArchive;
	}

	Vector3f getPosition() const { return m_Position; }
	Vector3f getRotation() const { return m_Rotation; }
	Vector3f getScale() const { return m_Scale; }

	void setPosition(const Vector3f inPosition) {
		m_Position = inPosition;
		setDirty();
	}

	void setRotation(const Vector3f inRotation) {
		m_Rotation = inRotation;
		setDirty();
	}

	void setScale(const Vector3f inScale) {
		m_Scale = inScale;
		setDirty();
	}

private:

	Vector3f m_Position{0, 0, 0};
	//TODO: always store as quaternion internally, and just convert so user doesnt have to use it
	Vector3f m_Rotation{0,0, 0};
	Vector3f m_Scale{1,1, 1};

};
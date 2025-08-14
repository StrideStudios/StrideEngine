#pragma once

#include <glm/ext/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include "Archive.h"
#include "Common.h"
#include "Engine.h"
#include "VulkanResourceManager.h"

struct SInstance {
	Matrix4f Transform = Matrix4f(1.f);

	friend CArchive& operator<<(CArchive& inArchive, const SInstance& inInstance) {
		inArchive << inInstance.Transform;
		return inArchive;
	}

	friend CArchive& operator>>(CArchive& inArchive, SInstance& inInstance) {
		inArchive >> inInstance.Transform;
		return inArchive;
	}
};

struct SInstancer {

	SInstancer(const uint32 initialSize = 0) {
		instances.resize(initialSize);
		setDirty();
	}

	~SInstancer() {
		instanceManager.flush();
	}

	void append(const std::vector<SInstance>& inInstances) {
		instances.append_range(inInstances);
		setDirty();
	}

	uint32 push(const SInstance& inInstance) {
		instances.push_back(inInstance);
		setDirty();
		return static_cast<uint32>(instances.size()) - 1;
	}

	SInstance remove(const uint32 inInstance) {
		const auto& instance = instances.erase(instances.begin() + inInstance);
		setDirty();
		return *instance;
	}

	void flush() {
		instances.clear();
		setDirty();
	}

	void reallocate(const Matrix4f& parentMatrix = Matrix4f(1.f));

	SBuffer_T* get(const Matrix4f& parentMatrix = Matrix4f(1.f)) {
		if (isDirty()) {
			mIsDirty = false;
			reallocate(parentMatrix);
		}
		return instanceBuffer;
	}

	bool isDirty() const {
		return mIsDirty;
	}

	void setDirty() {
		mIsDirty = true;
	}

	friend CArchive& operator<<(CArchive& inArchive, const SInstancer& inInstancer) {
		inArchive << inInstancer.instances;
		return inArchive;
	}

	friend CArchive& operator>>(CArchive& inArchive, SInstancer& inInstancer) {
		inArchive >> inInstancer.instances;
		return inArchive;
	}

	bool mIsDirty = true;

	CVulkanResourceManager instanceManager;

	std::vector<SInstance> instances;

private:

	SBuffer_T* instanceBuffer = nullptr;

};

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

	SInstancer m_Instancer{1}; //TODO: bad, shouldnt have an instancer in a non-instanced scene obj

	virtual SInstancer& getInstancer() {
		return m_Instancer;
	}

	virtual void onPositionChanged() {
		m_Instancer.setDirty();
	}

	std::string mName{"Scene Object"};

};

class CSceneObject2D : public CSceneObject {

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

class CSceneObject3D : public CSceneObject {

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
		onPositionChanged();
	}

	void setRotation(const Vector3f inRotation) {
		m_Rotation = inRotation;
		onPositionChanged();
	}

	void setScale(const Vector3f inScale) {
		m_Scale = inScale;
		onPositionChanged();
	}

private:

	Vector3f m_Position{0, 0, 0};
	//TODO: always store as quaternion internally, and just convert so user doesnt have to use it
	Vector3f m_Rotation{0,0, 0};
	Vector3f m_Scale{1,1, 1};

};
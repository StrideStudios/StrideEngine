#pragma once

#include "core/Archive.h"
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

	EXPORT SInstancer(uint32 initialSize = 0);

	EXPORT ~SInstancer();

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

	EXPORT void reallocate(const Matrix4f& parentMatrix = Matrix4f(1.f));

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

	std::vector<SInstance> instances;

private:

	CVulkanResourceManager m_ResourceManager;

	SBuffer_T* instanceBuffer = nullptr;

};
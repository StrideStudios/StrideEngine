#pragma once

#include "VulkanResources.h"
#include "core/Archive.h"

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

	EXPORT void destroy();

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

	//TODO: don't return SBuffer_T*
	SBuffer_T* get(const Matrix4f& parentMatrix = Matrix4f(1.f)) {
		if (isDirty()) {
			mIsDirty = false;
			reallocate(parentMatrix);
		}
		return instanceBuffer.get();
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

	SDynamicBuffer<VMA_MEMORY_USAGE_GPU_ONLY, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT> instanceBuffer;

};
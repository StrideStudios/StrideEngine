#pragma once

#include <vulkan/vulkan_core.h>

#include "renderer/VulkanResourceManager.h"

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

class CPass : public IDestroyable {

public:

	CPass(const std::string& inPassName): m_Name(inPassName) {}

	virtual void render(VkCommandBuffer cmd) = 0;

	void bindPipeline(VkCommandBuffer cmd, CPipeline* inPipeline, const struct SPushConstants& inConstants);

private:

	std::string m_Name;

	CPipeline* m_CurrentPipeline = nullptr;
};

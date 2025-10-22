#pragma once

#include <array>

#include "rendercore/VulkanResources.h"
#include "basic/core/Archive.h"

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

struct IInstancer : TDirtyable<true> {
	virtual size_t getNumberOfInstances() = 0;
	virtual SBuffer_T* get(const Matrix4f& parentMatrix = Matrix4f(1.f)) = 0;
};

struct SInstancer : IInstancer {

	EXPORT SInstancer(uint32 initialSize = 0);

	virtual size_t getNumberOfInstances() override {
		return instances.size();
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

	EXPORT void reallocate(const Matrix4f& parentMatrix = Matrix4f(1.f));

	//TODO: don't return SBuffer_T*
	virtual SBuffer_T* get(const Matrix4f& parentMatrix = Matrix4f(1.f)) override {
		if (isDirty()) {
			clean();
			reallocate(parentMatrix);
		}
		return instanceBuffer.get();
	}

	friend CArchive& operator<<(CArchive& inArchive, const SInstancer& inInstancer) {
		inArchive << inInstancer.instances;
		return inArchive;
	}

	friend CArchive& operator>>(CArchive& inArchive, SInstancer& inInstancer) {
		inArchive >> inInstancer.instances;
		return inArchive;
	}

	std::vector<SInstance> instances;

private:

	SDynamicBuffer<VMA_MEMORY_USAGE_GPU_ONLY, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT> instanceBuffer;

};

template <size_t TInstances>
struct SStaticInstancer : IInstancer {

	SStaticInstancer() {
		m_Instances.fill(SInstance{});
		setDirty();
	}

	virtual size_t getNumberOfInstances() override {
		return m_Instances.size();
	}

	void reallocate(const Matrix4f& parentMatrix = Matrix4f(1.f)) {

		for (auto& instance : m_Instances) {
			instance.Transform = parentMatrix * instance.Transform;
		}

		m_InstanceBuffer.push(m_Instances.data());
	}

	//TODO: don't return SBuffer_T*
	virtual SBuffer_T* get(const Matrix4f& parentMatrix = Matrix4f(1.f)) override {
		if (isDirty()) {
			clean();
			reallocate(parentMatrix);
		}
		return m_InstanceBuffer.get();
	}

	SInstance& getInstance(const size_t index = 0) {
		if (index >= TInstances) {
			errs("Invalid Instance Index {} in static instancer of size {}!", index, TInstances);
		}
		return m_Instances[index];
	}

	friend CArchive& operator<<(CArchive& inArchive, const SStaticInstancer& inInstancer) {
		inArchive << inInstancer.m_Instances;
		return inArchive;
	}

	friend CArchive& operator>>(CArchive& inArchive, SStaticInstancer& inInstancer) {
		inArchive >> inInstancer.m_Instances;
		return inArchive;
	}

private:

	std::array<SInstance, TInstances> m_Instances;

	SStaticBuffer<VMA_MEMORY_USAGE_GPU_ONLY, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, sizeof(SInstance), TInstances> m_InstanceBuffer;

};

typedef SStaticInstancer<1> SSingleInstancer;

template <>
struct SStaticInstancer<1> : IInstancer {

	SStaticInstancer() {
		setDirty();
	}

	virtual size_t getNumberOfInstances() override {
		return 1;
	}

	void reallocate(const Matrix4f& parentMatrix = Matrix4f(1.f)) {

		m_Instance.Transform = parentMatrix * m_Instance.Transform;

		m_InstanceBuffer.push(&m_Instance, sizeof(m_Instance));
	}

	//TODO: don't return SBuffer_T*
	virtual SBuffer_T* get(const Matrix4f& parentMatrix = Matrix4f(1.f)) override {
		if (isDirty()) {
			clean();
			reallocate(parentMatrix);
		}
		return m_InstanceBuffer.get();
	}

	SInstance& getInstance() {
		return m_Instance;
	}

	friend CArchive& operator<<(CArchive& inArchive, const SStaticInstancer& inInstancer) {
		inArchive << inInstancer.m_Instance;
		return inArchive;
	}

	friend CArchive& operator>>(CArchive& inArchive, SStaticInstancer& inInstancer) {
		inArchive >> inInstancer.m_Instance;
		return inArchive;
	}

private:

	SInstance m_Instance{};

	SStaticBuffer<VMA_MEMORY_USAGE_GPU_ONLY, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, sizeof(SInstance), 1> m_InstanceBuffer;

};
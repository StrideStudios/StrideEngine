#pragma once

#include <array>

#include "rendercore/RenderStack.h"
#include "rendercore/VulkanResources.h"
#include "basic/core/Archive.h"

struct SInstance {
	Matrix4f Transform{1.f};

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
	virtual SBuffer_T* get(SRenderStack& stack) = 0;
	virtual void flush() = 0;
};

template <size_t TInstances>
struct SStaticInstancer : IInstancer {

	SStaticInstancer() {
		m_Instances.fill(SInstance{});
	}

	virtual ~SStaticInstancer() override {
		msgs("Destroyed SStaticInstancer.");
		m_InstanceBuffer.destroy();
	}

	virtual size_t getNumberOfInstances() override {
		return m_Instances.size();
	}

	void reallocate(SRenderStack& stack) {

		for (auto& instance : m_Instances) {
			stack.push(instance.Transform);
			instance.Transform = stack.get();
			stack.pop();
		}

		m_InstanceBuffer.push(m_Instances.data());
	}

	//TODO: don't return SBuffer_T*
	virtual SBuffer_T* get(SRenderStack& stack) override {
		if (isDirty()) {
			clean();
			reallocate(stack);
		}
		return m_InstanceBuffer.get();
	}

	virtual void flush() override {
		m_Instances.clear();
		setDirty();
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

	SStaticBuffer<VMA_MEMORY_USAGE_GPU_ONLY, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, sizeof(SInstance), TInstances> m_InstanceBuffer{"Instances Buffer"};

};


struct SSingleInstancer : IInstancer {

	virtual ~SSingleInstancer() override {
		msgs("Destroyed SSingleInstancer.");
		m_InstanceBuffer.destroy();
	}

	virtual size_t getNumberOfInstances() override {
		return 1;
	}

	void reallocate(SRenderStack& stack) {

		stack.push(m_Instance.Transform);
		m_Instance.Transform = stack.get();
		stack.pop();

		m_InstanceBuffer.push(&m_Instance, sizeof(m_Instance));
	}

	//TODO: don't return SBuffer_T*
	virtual SBuffer_T* get(SRenderStack& stack) override {
		if (isDirty()) {
			clean();
			reallocate(stack);
		}
		return m_InstanceBuffer.get();
	}

	virtual void flush() override {
		setDirty();
	}

	SInstance& getInstance() {
		return m_Instance;
	}

	friend CArchive& operator<<(CArchive& inArchive, const SSingleInstancer& inInstancer) {
		inArchive << inInstancer.m_Instance;
		return inArchive;
	}

	friend CArchive& operator>>(CArchive& inArchive, SSingleInstancer& inInstancer) {
		inArchive >> inInstancer.m_Instance;
		return inArchive;
	}

private:

	SInstance m_Instance{};

	SStaticBuffer<VMA_MEMORY_USAGE_GPU_ONLY, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, sizeof(SInstance), 1> m_InstanceBuffer{"Instance Buffer"};

};

struct SDynamicInstancer : IInstancer {

	virtual ~SDynamicInstancer() override {
		msgs("Destroyed SDynamicInstancer.");
		m_InstanceBuffer.destroy();
	}

	virtual size_t getNumberOfInstances() override {
		return m_Instances.size();
	}

	void reallocate(SRenderStack& stack) {

		for (auto& instance : m_Instances) {
			stack.push(instance.Transform);
			instance.Transform = stack.get();
			stack.pop();
		}

		const size_t bufferSize = m_Instances.size() * sizeof(SInstance);

		m_InstanceBuffer.push(m_Instances.data(), bufferSize);
	}

	//TODO: don't return SBuffer_T*
	virtual SBuffer_T* get(SRenderStack& stack) override {
		if (isDirty()) {
			clean();
			reallocate(stack);
		}
		return m_InstanceBuffer.get();
	}

	virtual void flush() override {
		m_Instances.clear();
		setDirty();
	}

	size_t addInstance(const SInstance& inInstance) {
		m_Instances.push_back(inInstance);
		return m_Instances.size() - 1;
	}

	void removeInstance(const size_t index) {
		m_Instances.erase(m_Instances.begin() + index);
	}

	SInstance& getInstance(const size_t index = 0) {
		if (index >= m_Instances.size()) {
			errs("Invalid Instance Index {} in dynamic instancer of size {}!", index, m_Instances.size());
		}
		return m_Instances[index];
	}

	friend CArchive& operator<<(CArchive& inArchive, const SDynamicInstancer& inInstancer) {
		inArchive << inInstancer.m_Instances;
		return inArchive;
	}

	friend CArchive& operator>>(CArchive& inArchive, SDynamicInstancer& inInstancer) {
		inArchive >> inInstancer.m_Instances;
		return inArchive;
	}

private:

	std::vector<SInstance> m_Instances;

	SDynamicBuffer<VMA_MEMORY_USAGE_GPU_ONLY, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT> m_InstanceBuffer{"Dynamic Instance Buffer"};

};
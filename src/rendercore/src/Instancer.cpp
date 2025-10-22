#include "Instancer.h"

SInstancer::SInstancer(const uint32 initialSize) {
	instances.resize(initialSize);
	setDirty();
}

void SInstancer::reallocate(const Matrix4f& parentMatrix) {

	for (auto& instance : instances) {
		instance.Transform = parentMatrix * instance.Transform;
	}

	const size_t bufferSize = instances.size() * sizeof(SInstance);

	instanceBuffer.push(instances.data(), bufferSize);
}
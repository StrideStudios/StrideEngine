#include "Instancer.h"

SInstancer::SInstancer(const uint32 initialSize) {
	instances.resize(initialSize);
	setDirty();
}

SInstancer::~SInstancer() {
	destroy();
}

void SInstancer::destroy() {
	//TODO: is happening at end of execution because of shared ptr.
	instances.clear();
}

void SInstancer::reallocate(const Matrix4f& parentMatrix) {

	std::vector<SInstance> inputData = instances;

	for (auto& instance : inputData) {
		instance.Transform = parentMatrix * instance.Transform;
	}

	const size_t bufferSize = inputData.size() * sizeof(SInstance);

	instanceBuffer.push(inputData.data(), bufferSize);
}
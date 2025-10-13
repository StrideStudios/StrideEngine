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

	instanceBuffer.allocate(bufferSize);

	// Staging is not needed outside of this function
	SStaticBuffer<VMA_MEMORY_USAGE_CPU_ONLY, VK_BUFFER_USAGE_TRANSFER_SRC_BIT> staging{bufferSize};

	void* data = staging.get()->getMappedData();
	memcpy(data, inputData.data(), bufferSize);

	CRenderer::get()->immediateSubmit([&](SCommandBuffer& cmd) {
		VkBufferCopy vertexCopy{};
		vertexCopy.dstOffset = 0;
		vertexCopy.srcOffset = 0;
		vertexCopy.size = bufferSize;

		vkCmdCopyBuffer(cmd, staging.get()->buffer, instanceBuffer.get()->buffer, 1, &vertexCopy);
	});

	staging.destroy();
}
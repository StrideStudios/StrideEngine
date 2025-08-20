#include "VulkanResources.h"

#include "VulkanResourceManager.h"

CVulkanResourceManager gInstancerManager;

SInstancer::SInstancer(const uint32 initialSize) {
	instances.resize(initialSize);
	setDirty();
}

SInstancer::~SInstancer() {

	//TODO: Temporary wait until proper destruction
	CRenderer::get()->wait();

	gInstancerManager.flush();
}

void SInstancer::reallocate(const Matrix4f& parentMatrix) {

	std::vector<SInstance> inputData = instances;

	for (auto& instance : inputData) {
		instance.Transform = parentMatrix * instance.Transform;
	}

	const size_t bufferSize = inputData.size() * sizeof(SInstance);

	// Reallocate if buffer size has changed
	if (!instanceBuffer || instanceBuffer->info.size != bufferSize) {
		gInstancerManager.flush();

		instanceBuffer = gInstancerManager.allocateBuffer(bufferSize, VMA_MEMORY_USAGE_GPU_ONLY, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
	}

	// Staging is not needed outside of this function
	CVulkanResourceManager manager;
	const SBuffer_T* staging = manager.allocateBuffer(bufferSize, VMA_MEMORY_USAGE_CPU_ONLY, VK_BUFFER_USAGE_TRANSFER_SRC_BIT);

	void* data = staging->GetMappedData();
	memcpy(data, inputData.data(), bufferSize);

	// TODO: shouldnt have instancer here, but its necessary for instanced static mesh object...
	CRenderer::get()->immediateSubmit([&](SCommandBuffer& cmd) {
		VkBufferCopy vertexCopy{};
		vertexCopy.dstOffset = 0;
		vertexCopy.srcOffset = 0;
		vertexCopy.size = bufferSize;

		vkCmdCopyBuffer(cmd, staging->buffer, instanceBuffer->buffer, 1, &vertexCopy);
	});

	manager.flush();
}

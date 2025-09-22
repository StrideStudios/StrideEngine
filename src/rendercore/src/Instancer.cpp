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

	//TODO: Temporary wait until proper destruction
	vkDeviceWaitIdle(CRenderer::vkDevice());

	m_ResourceManager.flush();
	instances.clear();
}

void SInstancer::reallocate(const Matrix4f& parentMatrix) {

	std::vector<SInstance> inputData = instances;

	for (auto& instance : inputData) {
		instance.Transform = parentMatrix * instance.Transform;
	}

	const size_t bufferSize = inputData.size() * sizeof(SInstance);

	// Reallocate if buffer size has changed
	if (!instanceBuffer || instanceBuffer->info.size != bufferSize) {
		//TODO: Temporary wait until proper destruction
		vkDeviceWaitIdle(CRenderer::vkDevice());

		m_ResourceManager.flush();

		instanceBuffer = m_ResourceManager.allocateBuffer(bufferSize, VMA_MEMORY_USAGE_GPU_ONLY, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
	}

	// Staging is not needed outside of this function
	CVulkanResourceManager manager;
	const SBuffer_T* staging = manager.allocateBuffer(bufferSize, VMA_MEMORY_USAGE_CPU_ONLY, VK_BUFFER_USAGE_TRANSFER_SRC_BIT);

	void* data = staging->getMappedData();
	memcpy(data, inputData.data(), bufferSize);

	CRenderer::get()->immediateSubmit([&](SCommandBuffer& cmd) {
		VkBufferCopy vertexCopy{};
		vertexCopy.dstOffset = 0;
		vertexCopy.srcOffset = 0;
		vertexCopy.size = bufferSize;

		vkCmdCopyBuffer(cmd, staging->buffer, instanceBuffer->buffer, 1, &vertexCopy);
	});

	manager.flush();
}
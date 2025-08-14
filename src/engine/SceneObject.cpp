#include "SceneObject.h"

#include "Viewport.h"
#include "VulkanRenderer.h"

void SInstancer::reallocate(const Matrix4f& parentMatrix)  {

	std::vector<SInstance> inputData = instances;

	for (auto& instance : inputData) {
		instance.Transform = parentMatrix * instance.Transform;
	}

	instanceManager.flush();

	const size_t bufferSize = inputData.size() * sizeof(SInstance);

	instanceBuffer = instanceManager.allocateBuffer(bufferSize, VMA_MEMORY_USAGE_GPU_ONLY, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);

	// Staging is not needed outside of this function
	CVulkanResourceManager manager;
	const SBuffer_T* staging = manager.allocateBuffer(bufferSize, VMA_MEMORY_USAGE_CPU_ONLY, VK_BUFFER_USAGE_TRANSFER_SRC_BIT);

	void* data = staging->GetMappedData();
	memcpy(data, inputData.data(), bufferSize);

	CVulkanRenderer::immediateSubmit([&](VkCommandBuffer cmd) {
		VkBufferCopy vertexCopy{};
		vertexCopy.dstOffset = 0;
		vertexCopy.srcOffset = 0;
		vertexCopy.size = bufferSize;

		vkCmdCopyBuffer(cmd, staging->buffer, instanceBuffer->buffer, 1, &vertexCopy);
	});

	manager.flush();
}

Matrix4f CSceneObject2D::getTransformMatrix() const  {
	const Vector2f screenSize = CEngine::get().getViewport().mExtent;
	const Vector2f screenScale = m_Scale * screenSize;
	const Vector2f screenPos = m_Position * screenSize - m_Origin * screenScale;

	Matrix4f mat = glm::translate(Matrix4f(1.0), Vector3f(screenPos, 0.f));
	mat *= glm::mat4_cast(glm::qua(Vector3f(m_Rotation, 0.f, 0.f)));
	mat = glm::scale(mat, Vector3f(screenScale, 1.f));
	return mat;
}
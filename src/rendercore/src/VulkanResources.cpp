#include "VulkanResources.h"

#include "VulkanResourceManager.h"

CVulkanResourceManager gInstancerManager;

VkRenderingAttachmentInfo SRenderAttachment::get(const SImage_T* inImage) const {
	VkRenderingAttachmentInfo info {
		.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
		.pNext = nullptr,
		.loadOp = mLoadOp,
		.storeOp = mStoreOp
	};

	if (inImage) {
		info.imageView = inImage->mImageView;
		info.imageLayout = inImage->mLayout;
	}

	switch (mType) {
		case EAttachmentType::COLOR:
			info.clearValue = {
				.color = {
					.float32 = {
						mClearValue[0],
						mClearValue[1],
						mClearValue[2],
						mClearValue[3]
					}
				}
			};
			break;
		case EAttachmentType::DEPTH:
		case EAttachmentType::STENCIL:
			info.clearValue = {
				.depthStencil = {
					.depth = mClearValue[0],
					.stencil = static_cast<uint32>(mClearValue[1])
				}
			};
			break;
	}

	return info;
}

SInstancer::SInstancer(const uint32 initialSize) {
	instances.resize(initialSize);
	setDirty();
}

SInstancer::~SInstancer() {

	//TODO: Temporary wait until proper destruction
	vkDeviceWaitIdle(CRenderer::device());

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
		//TODO: Temporary wait until proper destruction
		vkDeviceWaitIdle(CRenderer::device());

		gInstancerManager.flush();

		instanceBuffer = gInstancerManager.allocateBuffer(bufferSize, VMA_MEMORY_USAGE_GPU_ONLY, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
	}

	// Staging is not needed outside of this function
	CVulkanResourceManager manager;
	const SBuffer_T* staging = manager.allocateBuffer(bufferSize, VMA_MEMORY_USAGE_CPU_ONLY, VK_BUFFER_USAGE_TRANSFER_SRC_BIT);

	void* data = staging->GetMappedData();
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

#include "VulkanResources.h"

#include "VulkanResourceManager.h"

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

void SBuffer_T::destroy() {
	vmaDestroyBuffer(CVulkanResourceManager::getAllocator(), buffer, allocation);
}

void* SBuffer_T::getMappedData() const {
	return CVulkanResourceManager::getMappedData(allocation);
}

void SBuffer_T::mapData(void** data) const {
	vmaMapMemory(CVulkanResourceManager::getAllocator(), allocation, data);
}

void SBuffer_T::unMapData() const {
	vmaUnmapMemory(CVulkanResourceManager::getAllocator(), allocation);
}

void SImage_T::destroy() {
	vmaDestroyImage(CVulkanResourceManager::getAllocator(), mImage, mAllocation);
	vkDestroyImageView(CRenderer::vkDevice(), mImageView, nullptr);
}
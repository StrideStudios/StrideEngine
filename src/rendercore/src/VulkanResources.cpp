#include "VulkanResources.h"

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

#include "rendercore/VulkanUtils.h"

#include <vulkan/vulkan_core.h>
#include <vulkan/vk_enum_string_helper.h>

#include "rendercore/VulkanResources.h"
#include "basic/Profiling.h"
#include "tracy/Tracy.hpp"

VkImageSubresourceRange CVulkanUtils::imageSubresourceRange(VkImageAspectFlags inAspectMask) {
	return {
		.aspectMask = inAspectMask,
		.baseMipLevel = 0,
		.levelCount = VK_REMAINING_MIP_LEVELS,
		.baseArrayLayer = 0,
		.layerCount = VK_REMAINING_ARRAY_LAYERS
	};
}

VkRenderingAttachmentInfo CVulkanUtils::createAttachmentInfo(VkImageView view, VkClearValue* clear ,VkImageLayout layout) {
	return {
		.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
		.pNext = nullptr,
		.imageView = view,
		.imageLayout = layout,
		.loadOp = clear ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD,
		.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
		.clearValue = clear ? *clear : VkClearValue()
	};
}

VkRenderingAttachmentInfo CVulkanUtils::createDepthAttachmentInfo(VkImageView view, VkImageLayout layout) {
	return {
		.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
		.pNext = nullptr,
		.imageView = view,
		.imageLayout = layout,
		.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
		.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
		.clearValue = {
			.depthStencil = {
				.depth = 0.f
			}
		}
	};
}

VkRenderingInfo CVulkanUtils::createRenderingInfo(VkExtent2D inExtent, VkRenderingAttachmentInfo* inColorAttachement, VkRenderingAttachmentInfo* inDepthAttachement) {
	return {
		.sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
		.pNext = nullptr,
		.renderArea = VkRect2D { VkOffset2D { 0, 0 }, inExtent },
		.layerCount = 1,
		.viewMask = 0,
		.colorAttachmentCount = 1,
		.pColorAttachments = inColorAttachement,
		.pDepthAttachment = inDepthAttachement,
		.pStencilAttachment = nullptr
	};
}

VkCommandPoolCreateInfo CVulkanInfo::createCommandPoolInfo(uint32 inQueueFamilyIndex, VkCommandPoolCreateFlags inFlags) {
	return {
	 	.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
		.pNext = nullptr,
		.flags = inFlags,
		.queueFamilyIndex = inQueueFamilyIndex
	};
}

VkCommandBufferAllocateInfo CVulkanInfo::createCommandAllocateInfo(VkCommandPool inPool, uint32 inCount ) {
	return {
	 	.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.pNext = nullptr,
		.commandPool = inPool,
		.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		.commandBufferCount = inCount
	};
}

VkFenceCreateInfo CVulkanInfo::createFenceInfo(VkFenceCreateFlags inFlags) {
	return {
	 	.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
		.pNext = nullptr,
		.flags = inFlags
	};
}

VkSemaphoreCreateInfo CVulkanInfo::createSemaphoreInfo(VkSemaphoreCreateFlags inFlags) {
	return {
	 	.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
	 	.pNext = nullptr,
	 	.flags = inFlags
	};
}

VkCommandBufferBeginInfo CVulkanInfo::createCommandBufferBeginInfo(VkCommandBufferUsageFlags inFlags) {
	return {
	 	.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.pNext = nullptr,
		.flags = inFlags,
		.pInheritanceInfo = nullptr
	};
}

VkSemaphoreSubmitInfo CVulkanInfo::submitSemaphoreInfo(VkPipelineStageFlags2 inStageMask, VkSemaphore inSemaphore) {
	return {
	 	.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
		.pNext = nullptr,
		.semaphore = inSemaphore,
		.value = 1,
		.stageMask = inStageMask,
		.deviceIndex = 0
	};
}

VkSubmitInfo2 CVulkanInfo::submitInfo(const VkCommandBufferSubmitInfo* inCmd, const VkSemaphoreSubmitInfo* inSignalSemaphoreInfo, const VkSemaphoreSubmitInfo* inWaitSemaphoreInfo) {
	return {
	 	.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
		.pNext = nullptr,

		.waitSemaphoreInfoCount = (uint32)(inWaitSemaphoreInfo == nullptr ? 0 : 1),
		.pWaitSemaphoreInfos = inWaitSemaphoreInfo,

		.commandBufferInfoCount = 1,
		.pCommandBufferInfos = inCmd,

		.signalSemaphoreInfoCount = (uint32)(inSignalSemaphoreInfo == nullptr ? 0 : 1),
		.pSignalSemaphoreInfos = inSignalSemaphoreInfo
	};
}

VkImageCreateInfo CVulkanInfo::createImageInfo(VkFormat inFormat, VkImageUsageFlags inUsageFlags, VkExtent3D inExtent) {
	return {
	 	.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
		.pNext = nullptr,

		.imageType = VK_IMAGE_TYPE_2D,

		.format = inFormat,
		.extent = inExtent,

		.mipLevels = 1,
		.arrayLayers = 1,

		//for MSAA. we will not be using it by default, so default it to 1 sample per pixel.
		.samples = VK_SAMPLE_COUNT_1_BIT,

		//optimal tiling, which means the image is stored on the best gpu format
		.tiling = VK_IMAGE_TILING_OPTIMAL,
		.usage = inUsageFlags,
		.sharingMode = VK_SHARING_MODE_EXCLUSIVE
	};
}

VkImageViewCreateInfo CVulkanInfo::createImageViewInfo(VkFormat inFormat, VkImage inImage, VkImageAspectFlags inAspectFlags) {
 	// build a image-view for the depth image to use for rendering
 	return {
 		.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
		 .pNext = nullptr,

		 .image = inImage,
		 .viewType = VK_IMAGE_VIEW_TYPE_2D,
		 .format = inFormat,
		 .subresourceRange = {
 			.aspectMask = inAspectFlags,
			 .baseMipLevel = 0,
			 .levelCount = 1,
			 .baseArrayLayer = 0,
			 .layerCount = 1
		 }
	};
}
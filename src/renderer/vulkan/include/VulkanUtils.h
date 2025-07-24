#pragma once

#include <vector>
#include <span>

#include "Common.h"
#include <vulkan/vulkan_core.h>
#include <vulkan/vk_enum_string_helper.h>

#define VK_CHECK(call) \
    if (auto vkResult = call; vkResult != VK_SUCCESS) { \
        err("{} Failed. Vulkan Error {}", #call, string_VkResult(vkResult)); \
    }

constexpr static uint8 gFrameOverlap = 2;

namespace CVulkanUtils {

	static VkImageSubresourceRange ImageSubresourceRange(VkImageAspectFlags inAspectMask) {
		return {
			.aspectMask = inAspectMask,
			.baseMipLevel = 0,
			.levelCount = VK_REMAINING_MIP_LEVELS,
			.baseArrayLayer = 0,
			.layerCount = VK_REMAINING_ARRAY_LAYERS
		};
	}

	static void CopyImageToImage(VkCommandBuffer inCmd, VkImage inSource, VkImage inDestination, VkExtent2D inSrcSize, VkExtent2D inDstSize) {
		VkImageBlit2 blitRegion { .sType = VK_STRUCTURE_TYPE_IMAGE_BLIT_2, .pNext = nullptr };

		blitRegion.srcOffsets[1].x = inSrcSize.width;
		blitRegion.srcOffsets[1].y = inSrcSize.height;
		blitRegion.srcOffsets[1].z = 1;

		blitRegion.dstOffsets[1].x = inDstSize.width;
		blitRegion.dstOffsets[1].y = inDstSize.height;
		blitRegion.dstOffsets[1].z = 1;

		blitRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		blitRegion.srcSubresource.baseArrayLayer = 0;
		blitRegion.srcSubresource.layerCount = 1;
		blitRegion.srcSubresource.mipLevel = 0;

		blitRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		blitRegion.dstSubresource.baseArrayLayer = 0;
		blitRegion.dstSubresource.layerCount = 1;
		blitRegion.dstSubresource.mipLevel = 0;

		VkBlitImageInfo2 blitInfo { .sType = VK_STRUCTURE_TYPE_BLIT_IMAGE_INFO_2, .pNext = nullptr };
		blitInfo.dstImage = inDestination;
		blitInfo.dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		blitInfo.srcImage = inSource;
		blitInfo.srcImageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		blitInfo.filter = VK_FILTER_LINEAR;
		blitInfo.regionCount = 1;
		blitInfo.pRegions = &blitRegion;

		vkCmdBlitImage2(inCmd, &blitInfo);
	}

	static void TransitionImage(VkCommandBuffer inCmd, VkImage inImage, VkImageLayout inCurrentLayout, VkImageLayout inNewLayout) {
		VkImageMemoryBarrier2 imageBarrier {.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2};
		imageBarrier.pNext = nullptr;

		// TODO: apparently ALL_COMMANDS_BIT is slow, and will stall the GPU if using many transitions
		imageBarrier.srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
		imageBarrier.srcAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT;
		imageBarrier.dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
		imageBarrier.dstAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT | VK_ACCESS_2_MEMORY_READ_BIT;

		imageBarrier.oldLayout = inCurrentLayout;
		imageBarrier.newLayout = inNewLayout;

		VkImageAspectFlags aspectMask = (inNewLayout == VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL) ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
		imageBarrier.subresourceRange = ImageSubresourceRange(aspectMask);
		imageBarrier.image = inImage;

		VkDependencyInfo depInfo {
			.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
			.pNext = nullptr,
			.imageMemoryBarrierCount = 1,
			.pImageMemoryBarriers = &imageBarrier
		};

		vkCmdPipelineBarrier2(inCmd, &depInfo);
	}

	static VkRenderingAttachmentInfo createAttachmentInfo(VkImageView view, VkClearValue* clear ,VkImageLayout layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL) {
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

	static VkRenderingInfo createRenderingInfo(VkExtent2D inExtent, VkRenderingAttachmentInfo* inColorAttachement, VkRenderingAttachmentInfo* inDepthAttachement) {
		return {
			.sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
			.pNext = nullptr,
			.renderArea = VkRect2D { VkOffset2D { 0, 0 }, inExtent },
			.layerCount = 1,
			.colorAttachmentCount = 1,
			.pColorAttachments = inColorAttachement,
			.pDepthAttachment = inDepthAttachement,
			.pStencilAttachment = nullptr
		};
	}
}

namespace CVulkanInfo {

	static VkCommandPoolCreateInfo createCommandPoolInfo(uint32 inQueueFamilyIndex, VkCommandPoolCreateFlags inFlags = 0) {
		return {
			.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
			.pNext = nullptr,
			.flags = inFlags,
			.queueFamilyIndex = inQueueFamilyIndex
		};
	}

	static VkCommandBufferAllocateInfo createCommandAllocateInfo(VkCommandPool inPool, uint32 inCount = 1) {
		return {
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
			.pNext = nullptr,
			.commandPool = inPool,
			.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
			.commandBufferCount = inCount
		};
	}

	static VkFenceCreateInfo createFenceInfo(VkFenceCreateFlags inFlags = 0) {
		return {
			.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
			.pNext = nullptr,
			.flags = inFlags
		};
	}

	static VkSemaphoreCreateInfo createSemaphoreInfo(VkSemaphoreCreateFlags inFlags = 0) {
		return {
			.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
			.pNext = nullptr,
			.flags = inFlags
		};
	}

	static VkCommandBufferBeginInfo createCommandBufferBeginInfo(VkCommandBufferUsageFlags inFlags = 0) {
		return {
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
			.pNext = nullptr,
			.flags = inFlags,
			.pInheritanceInfo = nullptr
		};
	}

	static VkSemaphoreSubmitInfo submitSemaphoreInfo(VkPipelineStageFlags2 inStageMask, VkSemaphore inSemaphore) {
		return {
			.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
			.pNext = nullptr,
			.semaphore = inSemaphore,
			.value = 1,
			.stageMask = inStageMask,
			.deviceIndex = 0
		};
	}

	static VkCommandBufferSubmitInfo submitCommandBufferInfo(VkCommandBuffer inCmd) {
		return {
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
			.pNext = nullptr,
			.commandBuffer = inCmd,
			.deviceMask = 0
		};
	}

	static VkSubmitInfo submitInfo(VkCommandBuffer* inCmd) {
		return {
			.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
			.pNext = nullptr,
			.commandBufferCount = 1,
			.pCommandBuffers = inCmd
		};
	}

	static VkSubmitInfo2 submitInfo(const VkCommandBufferSubmitInfo* inCmd, const VkSemaphoreSubmitInfo* inSignalSemaphoreInfo, const VkSemaphoreSubmitInfo* inWaitSemaphoreInfo) {
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

	static VkImageCreateInfo createImageInfo(VkFormat inFormat, VkImageUsageFlags inUsageFlags, VkExtent3D inExtent)
	{
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
			.usage = inUsageFlags
		};
	}

	static VkImageViewCreateInfo createImageViewInfo(VkFormat inFormat, VkImage inImage, VkImageAspectFlags inAspectFlags)
	{
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
}

struct SDescriptorLayoutBuilder {

	std::vector<VkDescriptorSetLayoutBinding> bindings;

	void addBinding(uint32 inBinding, VkDescriptorType inDescriptorType) {
		bindings.push_back({
			.binding = inBinding,
			.descriptorType = inDescriptorType,
			.descriptorCount = 1
		});
	}

	void clear() {
		bindings.clear();
	}

	VkDescriptorSetLayout build(VkDevice inDevice, VkShaderStageFlags inShaderStages, void* pNext = nullptr, VkDescriptorSetLayoutCreateFlags inFlags = 0) {
		for (auto& b : bindings) {
			b.stageFlags |= inShaderStages;
		}

		VkDescriptorSetLayoutCreateInfo info {
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
			.pNext = pNext,
			.flags = inFlags,
			.bindingCount = (uint32_t)bindings.size(),
			.pBindings = bindings.data()
		};

		VkDescriptorSetLayout set;
		VK_CHECK(vkCreateDescriptorSetLayout(inDevice, &info, nullptr, &set));

		return set;
	}
};

struct SDescriptorAllocator {

	struct PoolSizeRatio{
		VkDescriptorType mType;
		float mRatio;
	};

	VkDescriptorPool mPool;

	void init(VkDevice inDevice, uint32_t inMaxSets, std::span<PoolSizeRatio> inPoolRatios) {
		std::vector<VkDescriptorPoolSize> poolSizes;
		for (PoolSizeRatio ratio : inPoolRatios) {
			poolSizes.push_back(VkDescriptorPoolSize{
				.type = ratio.mType,
				.descriptorCount = uint32_t(ratio.mRatio * inMaxSets)
			});
		}

		VkDescriptorPoolCreateInfo pool_info {
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.maxSets = inMaxSets,
			.poolSizeCount = (uint32_t)poolSizes.size(),
			.pPoolSizes = poolSizes.data()
		};

		vkCreateDescriptorPool(inDevice, &pool_info, nullptr, &mPool);
	}

	void clear(VkDevice inDevice) {
		vkResetDescriptorPool(inDevice, mPool, 0);
	}

	void destroy(VkDevice inDevice) {
		vkDestroyDescriptorPool(inDevice,mPool,nullptr);
	}

	VkDescriptorSet allocate(VkDevice inDevice, VkDescriptorSetLayout inLayout) {
		VkDescriptorSetAllocateInfo allocInfo {
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
			.pNext = nullptr,
			.descriptorPool = mPool,
			.descriptorSetCount = 1,
			.pSetLayouts = &inLayout
		};

		VkDescriptorSet ds;
		VK_CHECK(vkAllocateDescriptorSets(inDevice, &allocInfo, &ds));

		return ds;
	}
};
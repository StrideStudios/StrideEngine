#pragma once

#include "Common.h"
#include <vulkan/vulkan_core.h>
#include <vma/vk_mem_alloc.h>

#define VK_CHECK(call) \
    if (auto vkResult = call; vkResult != VK_SUCCESS) { \
        err("{} Failed. Vulkan Error {}", #call, string_VkResult(vkResult)); \
    }

constexpr static uint8 gFrameOverlap = 2;

struct SAllocatedImage {
	VkImage mImage;
	VkImageView mImageView;
	VmaAllocation mAllocation;
	//VkExtent3D mImageExtent;
	VkFormat mImageFormat;
};

namespace CVulkanUtils {

	static VkImageSubresourceRange ImageSubresourceRange(VkImageAspectFlags inAspectMask) {
		VkImageSubresourceRange subImage {};
		subImage.aspectMask = inAspectMask;
		subImage.baseMipLevel = 0;
		subImage.levelCount = VK_REMAINING_MIP_LEVELS;
		subImage.baseArrayLayer = 0;
		subImage.layerCount = VK_REMAINING_ARRAY_LAYERS;

		return subImage;
	}

	static void CopyImageToImage(VkCommandBuffer inCmd, VkImage inSource, VkImage inDestination, VkExtent2D inSrcSize, VkExtent2D inDstSize)
	{
		VkImageBlit2 blitRegion{ .sType = VK_STRUCTURE_TYPE_IMAGE_BLIT_2, .pNext = nullptr };

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

		VkBlitImageInfo2 blitInfo{ .sType = VK_STRUCTURE_TYPE_BLIT_IMAGE_INFO_2, .pNext = nullptr };
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

		VkDependencyInfo depInfo {};
		depInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
		depInfo.pNext = nullptr;

		depInfo.imageMemoryBarrierCount = 1;
		depInfo.pImageMemoryBarriers = &imageBarrier;

		vkCmdPipelineBarrier2(inCmd, &depInfo);
	}
}

namespace CVulkanInfo {

	static VkCommandPoolCreateInfo CreateCommandPoolInfo(uint32 inQueueFamilyIndex, VkCommandPoolCreateFlags inFlags = 0) {
		VkCommandPoolCreateInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		info.pNext = nullptr;
		info.queueFamilyIndex = inQueueFamilyIndex;
		info.flags = inFlags;
		return info;
	}

	static VkCommandBufferAllocateInfo CreateCommandAllocateInfo(VkCommandPool inPool, uint32 inCount = 1) {
		VkCommandBufferAllocateInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		info.pNext = nullptr;

		info.commandPool = inPool;
		info.commandBufferCount = inCount;
		info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		return info;
	}

	static VkFenceCreateInfo CreateFenceInfo(VkFenceCreateFlags inFlags = 0) {
		VkFenceCreateInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		info.pNext = nullptr;

		info.flags = inFlags;

		return info;
	}

	static VkSemaphoreCreateInfo CreateSemaphoreInfo(VkSemaphoreCreateFlags inFlags = 0) {
		VkSemaphoreCreateInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
		info.pNext = nullptr;
		info.flags = inFlags;
		return info;
	}

	static VkCommandBufferBeginInfo CreateCommandBufferBeginInfo(VkCommandBufferUsageFlags inFlags = 0) {
		VkCommandBufferBeginInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		info.pNext = nullptr;

		info.pInheritanceInfo = nullptr;
		info.flags = inFlags;
		return info;
	}

	static VkSemaphoreSubmitInfo SubmitSemaphoreInfo(VkPipelineStageFlags2 inStageMask, VkSemaphore inSemaphore) {
		VkSemaphoreSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
		submitInfo.pNext = nullptr;
		submitInfo.semaphore = inSemaphore;
		submitInfo.stageMask = inStageMask;
		submitInfo.deviceIndex = 0;
		submitInfo.value = 1;

		return submitInfo;
	}

	static VkCommandBufferSubmitInfo SubmitCommandBufferInfo(VkCommandBuffer inCmd) {
		VkCommandBufferSubmitInfo info{};
		info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
		info.pNext = nullptr;
		info.commandBuffer = inCmd;
		info.deviceMask = 0;

		return info;
	}

	static VkSubmitInfo2 SubmitInfo(const VkCommandBufferSubmitInfo* inCmd, const VkSemaphoreSubmitInfo* inSignalSemaphoreInfo, const VkSemaphoreSubmitInfo* inWaitSemaphoreInfo) {
		VkSubmitInfo2 info = {};
		info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
		info.pNext = nullptr;

		info.waitSemaphoreInfoCount = inWaitSemaphoreInfo == nullptr ? 0 : 1;
		info.pWaitSemaphoreInfos = inWaitSemaphoreInfo;

		info.signalSemaphoreInfoCount = inSignalSemaphoreInfo == nullptr ? 0 : 1;
		info.pSignalSemaphoreInfos = inSignalSemaphoreInfo;

		info.commandBufferInfoCount = 1;
		info.pCommandBufferInfos = inCmd;

		return info;
	}

	static VkImageCreateInfo CreateImageInfo(VkFormat inFormat, VkImageUsageFlags inUsageFlags, VkExtent3D inExtent)
	{
		VkImageCreateInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		info.pNext = nullptr;

		info.imageType = VK_IMAGE_TYPE_2D;

		info.format = inFormat;
		info.extent = inExtent;

		info.mipLevels = 1;
		info.arrayLayers = 1;

		//for MSAA. we will not be using it by default, so default it to 1 sample per pixel.
		info.samples = VK_SAMPLE_COUNT_1_BIT;

		//optimal tiling, which means the image is stored on the best gpu format
		info.tiling = VK_IMAGE_TILING_OPTIMAL;
		info.usage = inUsageFlags;

		return info;
	}

	static VkImageViewCreateInfo CreateImageViewInfo(VkFormat inFormat, VkImage inImage, VkImageAspectFlags inAspectFlags)
	{
		// build a image-view for the depth image to use for rendering
		VkImageViewCreateInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		info.pNext = nullptr;

		info.viewType = VK_IMAGE_VIEW_TYPE_2D;
		info.image = inImage;
		info.format = inFormat;
		info.subresourceRange.baseMipLevel = 0;
		info.subresourceRange.levelCount = 1;
		info.subresourceRange.baseArrayLayer = 0;
		info.subresourceRange.layerCount = 1;
		info.subresourceRange.aspectMask = inAspectFlags;

		return info;
	}


}

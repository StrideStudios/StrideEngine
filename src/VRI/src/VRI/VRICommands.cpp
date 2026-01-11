#include "VRI/VRICommands.h"

#include <vulkan/vk_enum_string_helper.h>

#include "VRI/VRI.h"
#include "VRI/VRIResources.h"
#include "tracy/Tracy.hpp"

CVRICommands::CVRICommands(const TFrail<CCommandPool>& inCmdPool) {
	const VkCommandBufferAllocateInfo frameCmdAllocInfo {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
	   .pNext = nullptr,
	   .commandPool = inCmdPool->mCommandPool,
	   .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
	   .commandBufferCount = 1
   };
	VK_CHECK(vkAllocateCommandBuffers(CVRI::get()->getDevice()->device, &frameCmdAllocInfo, &cmd));
}

void CVRICommands::begin(const bool inReset) const {
	if (inReset) {
		VK_CHECK(vkResetCommandBuffer(cmd, 0));
	}

	constexpr VkCommandBufferBeginInfo cmdBeginInfo {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
	   .pNext = nullptr,
	   .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
	   .pInheritanceInfo = nullptr
   };
	VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));
}

void CVRICommands::end() {
	vkEndCommandBuffer(cmd);

	// Image transitions can no longer occur
	imageTransitions.forEach([](size_t, const TFrail<SVRIImage>& image) {
		image->mLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	});
	imageTransitions.clear();
}

void CVRICommands::bindPipeline(const TFrail<CPipeline>& pipeline, const VkPipelineBindPoint inBindPoint) const {
	vkCmdBindPipeline(cmd, inBindPoint, pipeline->mPipeline);
}

void CVRICommands::bindDescriptorSets(const TFrail<CDescriptorSet>& descriptorSet, VkPipelineBindPoint inBindPoint, VkPipelineLayout inPipelineLayout, uint32 inFirstSet, uint32 inDescriptorSetCount) const {
	vkCmdBindDescriptorSets(cmd, inBindPoint,inPipelineLayout, inFirstSet, inDescriptorSetCount, &descriptorSet->mDescriptorSet, 0, nullptr);
}

void CVRICommands::blitImage(const TFrail<SVRIImage>& inSource, const TFrail<SVRIImage>& inDestination, const Extent32u inSrcSize, const Extent32u inDstSize) const {
	ZoneScopedN("Image Blit");

	VkImageBlit2 blitRegion { .sType = VK_STRUCTURE_TYPE_IMAGE_BLIT_2, .pNext = nullptr };

	blitRegion.srcOffsets[1].x = inSrcSize.x;
	blitRegion.srcOffsets[1].y = inSrcSize.y;
	blitRegion.srcOffsets[1].z = 1;

	blitRegion.dstOffsets[1].x = inDstSize.x;
	blitRegion.dstOffsets[1].y = inDstSize.y;
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
	blitInfo.dstImage = inDestination->mImage;
	blitInfo.dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	blitInfo.srcImage = inSource->mImage;
	blitInfo.srcImageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
	blitInfo.filter = VK_FILTER_LINEAR;
	blitInfo.regionCount = 1;
	blitInfo.pRegions = &blitRegion;

	vkCmdBlitImage2(cmd, &blitInfo);
}

void CVRICommands::transitionImage(const TFrail<SVRIImage>& inImage, VkImageLayout inLayout) {
	ZoneScopedN("Transition Image");

	VkImageMemoryBarrier2 imageBarrier {.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2};
	imageBarrier.pNext = nullptr;

	// TODO: apparently ALL_COMMANDS_BIT is slow, and will stall the GPU if using many transitions
	imageBarrier.srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
	imageBarrier.srcAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT;
	imageBarrier.dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
	imageBarrier.dstAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT | VK_ACCESS_2_MEMORY_READ_BIT;

	imageBarrier.oldLayout = inImage->mLayout;
	imageBarrier.newLayout = inLayout;

	const VkImageAspectFlags aspectMask = (inLayout == VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL) ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
	imageBarrier.subresourceRange = VkImageSubresourceRange{
		.aspectMask = aspectMask,
		.baseMipLevel = 0,
		.levelCount = VK_REMAINING_MIP_LEVELS,
		.baseArrayLayer = 0,
		.layerCount = VK_REMAINING_ARRAY_LAYERS
	};
	imageBarrier.image = inImage->mImage;

	const VkDependencyInfo depInfo {
		.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
		.pNext = nullptr,
		.imageMemoryBarrierCount = 1,
		.pImageMemoryBarriers = &imageBarrier
	};

	vkCmdPipelineBarrier2(cmd, &depInfo);

	inImage->mLayout = inLayout;
	imageTransitions.push(inImage);
}

void CVRICommands::clearImage(const TFrail<SVRIImage>& inImage) const {
	constexpr VkClearColorValue color = { .float32 = {0.0, 0.0, 0.0} };
	constexpr VkImageSubresourceRange imageSubresourceRange { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
	vkCmdClearColorImage(cmd, inImage->mImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &color, 1, &imageSubresourceRange);
}

void CVRICommands::copyBufferToImage(const TFrail<SVRIBuffer>& inBuffer, const TFrail<SVRIImage>& inImage, VkImageLayout dstImageLayout, uint32_t regionCount, const VkBufferImageCopy& pRegion) const {
	vkCmdCopyBufferToImage(cmd, inBuffer->buffer, inImage->mImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &pRegion);
}

void CVRICommands::setViewportScissor(const Extent32u inExtent) const {
	const VkViewport viewport = {
		.x = 0,
		.y = 0,
		.width = (float)inExtent.x,
		.height = (float)inExtent.y,
		.minDepth = 0.f,
		.maxDepth = 1.f
	};


	vkCmdSetViewport(cmd, 0, 1, &viewport);

	const VkRect2D scissor = {
		.offset = {
			.x = 0,
			.y = 0
		},
		.extent = {
			.width = inExtent.x,
			.height = inExtent.y
		}
	};

	vkCmdSetScissor(cmd, 0, 1, &scissor);
}

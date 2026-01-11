#pragma once

#include <vulkan/vulkan_core.h>

#include "VRIResources.h"
#include "sstl/ForwardList.h"

// Commands used to interact with the vulkan driver
// TODO: separate thread?
class CVRICommands {

public:

	EXPORT CVRICommands(const TFrail<CCommandPool>& inCmdPool);

	//TODO: not all command buffers are one time submit, perhaps have a parent class
	EXPORT void begin(bool inReset = true) const;
	EXPORT void end();

	VkSubmitInfo submitInfo0() {
		return {
			.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		   .pNext = nullptr,
		   .commandBufferCount = 1,
		   .pCommandBuffers = &cmd
	   };
	}

	VkCommandBufferSubmitInfo submitInfo() const {
		return {
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
		   .pNext = nullptr,
		   .commandBuffer = cmd,
		   .deviceMask = 0
	   };
	}

	void beginRendering(const VkRenderingInfo& info) const {
		vkCmdBeginRendering(cmd, &info);
	}

	void endRendering() const {
		vkCmdEndRendering(cmd);
	}

	void pushConstants(VkPipelineLayout layout,
	VkShaderStageFlags stageFlags,
	uint32_t offset,
	uint32_t size,
	const void* pValues) const {
		vkCmdPushConstants(cmd, layout, stageFlags, offset, size, pValues);
	}

	void bindIndexBuffers(
	VkBuffer buffer,
	VkDeviceSize offset,
	VkIndexType indexType) const {
		vkCmdBindIndexBuffer(cmd, buffer, offset, indexType);
	}

	void bindVertexBuffers(
	const uint32_t firstBinding,
	const uint32_t bindingCount,
	const VkBuffer* pBuffers,
	const VkDeviceSize* pOffsets) const {
		vkCmdBindVertexBuffers(cmd, firstBinding, bindingCount, pBuffers, pOffsets);
	}

	void draw(uint32_t vertexCount,
	uint32_t instanceCount,
	uint32_t firstVertex,
	uint32_t firstInstance) {
		vkCmdDraw(cmd, vertexCount, instanceCount, firstVertex, firstInstance);
	}

	void drawIndexed( uint32_t indexCount,
	uint32_t instanceCount,
	uint32_t firstIndex,
	int32_t vertexOffset,
	uint32_t firstInstance) {
		vkCmdDrawIndexed(cmd, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
	}

	void copyBuffer(VkBuffer srcBuffer,
	VkBuffer dstBuffer,
	uint32_t regionCount,
	const VkBufferCopy& pRegions) {
		vkCmdCopyBuffer(cmd, srcBuffer, dstBuffer, regionCount, &pRegions);
	}

	EXPORT void bindPipeline(const TFrail<CPipeline>& pipeline, VkPipelineBindPoint inBindPoint) const;
	EXPORT void bindDescriptorSets(const TFrail<CDescriptorSet>& descriptorSet, VkPipelineBindPoint inBindPoint, VkPipelineLayout inPipelineLayout, uint32 inFirstSet, uint32 inDescriptorSetCount) const;

    /*
     * Image Commands
     */

    EXPORT void blitImage(const TFrail<SVRIImage>& inSource, const TFrail<SVRIImage>& inDestination, Extent32u inSrcSize, Extent32u inDstSize) const;
    EXPORT void transitionImage(const TFrail<SVRIImage>& inImage, VkImageLayout inLayout);
	EXPORT void clearImage(const TFrail<SVRIImage>& inImage) const;
	EXPORT void copyBufferToImage(const TFrail<SVRIBuffer>& inBuffer, const TFrail<SVRIImage>& inImage,  VkImageLayout dstImageLayout,
	uint32_t regionCount,
	const VkBufferImageCopy& pRegion) const;

	void blitImage2(const VkBlitImageInfo2& info) const {
		vkCmdBlitImage2(cmd, &info);
	}

	void pipelineBarrier2(const VkDependencyInfo& depInfo) const {
		vkCmdPipelineBarrier2(cmd, &depInfo);
	}

	/*
	 * Viewport Commands
	 */

	EXPORT void setViewportScissor(Extent32u inExtent) const;

//private:

	VkCommandBuffer cmd = nullptr;
	TForwardList<TFrail<SVRIImage>> imageTransitions;
};
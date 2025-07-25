#include "VulkanUtils.h"

#include <vulkan/vulkan_core.h>
#include <vulkan/vk_enum_string_helper.h>

#include "Common.h"
#include "Engine.h"
#include "VulkanDevice.h"

VkImageSubresourceRange CVulkanUtils::imageSubresourceRange(VkImageAspectFlags inAspectMask) {
	return {
		.aspectMask = inAspectMask,
		.baseMipLevel = 0,
		.levelCount = VK_REMAINING_MIP_LEVELS,
		.baseArrayLayer = 0,
		.layerCount = VK_REMAINING_ARRAY_LAYERS
	};
}

void CVulkanUtils::copyImageToImage(VkCommandBuffer inCmd, VkImage inSource, VkImage inDestination, VkExtent2D inSrcSize, VkExtent2D inDstSize) {
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

void CVulkanUtils::transitionImage(VkCommandBuffer inCmd, VkImage inImage, VkImageLayout inCurrentLayout, VkImageLayout inNewLayout) {
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
	imageBarrier.subresourceRange = imageSubresourceRange(aspectMask);
	imageBarrier.image = inImage;

	VkDependencyInfo depInfo {
		.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
		.pNext = nullptr,
		.imageMemoryBarrierCount = 1,
		.pImageMemoryBarriers = &imageBarrier
	};

	vkCmdPipelineBarrier2(inCmd, &depInfo);
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

VkCommandBufferSubmitInfo CVulkanInfo::submitCommandBufferInfo(VkCommandBuffer inCmd) {
	return {
	 	.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
		.pNext = nullptr,
		.commandBuffer = inCmd,
		.deviceMask = 0
	};
}

VkSubmitInfo CVulkanInfo::submitInfo(VkCommandBuffer* inCmd) {
	return {
	 	.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.pNext = nullptr,
		.commandBufferCount = 1,
		.pCommandBuffers = inCmd
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
		.usage = inUsageFlags
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

void SDescriptorLayoutBuilder::addBinding(uint32 inBinding, VkDescriptorType inDescriptorType) {
	bindings.push_back({
		.binding = inBinding,
		.descriptorType = inDescriptorType,
		.descriptorCount = 1
	});
}

void SDescriptorLayoutBuilder::clear() {
	bindings.clear();
}

VkDescriptorSetLayout SDescriptorLayoutBuilder::build(VkShaderStageFlags inShaderStages, void *pNext, VkDescriptorSetLayoutCreateFlags inFlags) {
	 for (auto& b : bindings) {
	 	b.stageFlags |= inShaderStages;
	 }

 	VkDescriptorSetLayoutCreateInfo info {
 		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		 .pNext = pNext,
		 .flags = inFlags,
		 .bindingCount = static_cast<uint32>(bindings.size()),
		 .pBindings = bindings.data()
	 };

 	VkDescriptorSetLayout set;
 	VK_CHECK(vkCreateDescriptorSetLayout(CEngine::device(), &info, nullptr, &set));

	return set;
}

void SDescriptorAllocator::init(uint32_t inMaxSets, std::span<PoolSizeRatio> inPoolRatios) {
	ratios.clear();

	for (auto r : inPoolRatios) {
		ratios.push_back(r);
	}

	VkDescriptorPool newPool = createPool(inMaxSets, inPoolRatios);

	setsPerPool = static_cast<uint32>(inMaxSets * 1.5);

	readyPools.push_back(newPool);
}

void SDescriptorAllocator::clear() {
	for (const auto pool : readyPools) {
		vkResetDescriptorPool(CEngine::device(), pool, 0);
	}
	for (auto pool : fullPools) {
		vkResetDescriptorPool(CEngine::device(), pool, 0);
		readyPools.push_back(pool);
	}
	fullPools.clear();
}

void SDescriptorAllocator::destroy() {
	for (const auto pool : readyPools) {
		vkDestroyDescriptorPool(CEngine::device(), pool, nullptr);
	}
	readyPools.clear();
	for (const auto pool : fullPools) {
		vkDestroyDescriptorPool(CEngine::device(),pool,nullptr);
	}
	fullPools.clear();
}

VkDescriptorSet SDescriptorAllocator::allocate(VkDescriptorSetLayout inLayout, void* pNext) {
	// Get or create a pool to allocate from
	VkDescriptorPool poolToUse = getPool();

	VkDescriptorSetAllocateInfo allocInfo = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		.pNext = pNext,
		.descriptorPool = poolToUse,
		.descriptorSetCount = 1,
		.pSetLayouts = &inLayout
	};

	VkDescriptorSet ds;

	// Allocation failed. Try again
	if (const VkResult result = vkAllocateDescriptorSets(CEngine::device(), &allocInfo, &ds); result == VK_ERROR_OUT_OF_POOL_MEMORY || result == VK_ERROR_FRAGMENTED_POOL) {
		fullPools.push_back(poolToUse);

		poolToUse = getPool();
		allocInfo.descriptorPool = poolToUse;

		VK_CHECK(vkAllocateDescriptorSets(CEngine::device(), &allocInfo, &ds));
	}

	readyPools.push_back(poolToUse);
	return ds;
}

VkDescriptorPool SDescriptorAllocator::getPool() {
	VkDescriptorPool newPool;
	if (!readyPools.empty()) {
		newPool = readyPools.back();
		readyPools.pop_back();
	} else {
		// Create new pool
		newPool = createPool(setsPerPool, ratios);

		setsPerPool = uint32(setsPerPool * 1.5);

		if (setsPerPool > 4092) {
			setsPerPool = 4092;
		}
	}
	return newPool;
}

VkDescriptorPool SDescriptorAllocator::createPool(const uint32 inSetCount, const std::span<PoolSizeRatio> inPoolRatios) {
	std::vector<VkDescriptorPoolSize> poolSizes;
	for (auto [mType, mRatio] : inPoolRatios) {
		poolSizes.push_back(VkDescriptorPoolSize{
			.type = mType,
			.descriptorCount = static_cast<uint32>(mRatio * inSetCount)
		});
	}

	VkDescriptorPoolCreateInfo poolInfo {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.maxSets = inSetCount,
		.poolSizeCount = (uint32)poolSizes.size(),
		.pPoolSizes = poolSizes.data()
	};

	VkDescriptorPool newPool;
	vkCreateDescriptorPool(CEngine::device(), &poolInfo, nullptr, &newPool);
	return newPool;
}

//TODO: info in SImage then just pass in here
void SDescriptorWriter::writeImage(uint32 inBinding, VkImageView inImage, VkSampler inSampler, VkImageLayout inLayout, VkDescriptorType inType) {
	VkDescriptorImageInfo& info = imageInfos.emplace_back(VkDescriptorImageInfo{
		.sampler = inSampler,
		.imageView = inImage,
		.imageLayout = inLayout
	});

	writes.push_back({
		.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		.dstSet = VK_NULL_HANDLE, //left empty for now until we need to write it
		.dstBinding = inBinding,
		.descriptorCount = 1,
		.descriptorType = inType,
		.pImageInfo = &info
	});
}

//TODO: pass in SBuffer here
void SDescriptorWriter::writeBuffer(uint32 inBinding, VkBuffer inBuffer, size_t inSize, size_t inOffset, VkDescriptorType inType) {
	VkDescriptorBufferInfo& info = bufferInfos.emplace_back(VkDescriptorBufferInfo{
		.buffer = inBuffer,
		.offset = inOffset,
		.range = inSize
	});

	writes.push_back({
		.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		.dstSet = VK_NULL_HANDLE, //left empty for now until we need to write it
		.dstBinding = inBinding,
		.descriptorCount = 1,
		.descriptorType = inType,
		.pBufferInfo = &info
	});
}

void SDescriptorWriter::clear() {
	imageInfos.clear();
	writes.clear();
	bufferInfos.clear();
}

void SDescriptorWriter::updateSet(VkDescriptorSet inSet) {
	for (VkWriteDescriptorSet& write : writes) {
		write.dstSet = inSet;
	}

	vkUpdateDescriptorSets(CEngine::device(), (uint32)writes.size(), writes.data(), 0, nullptr);
}
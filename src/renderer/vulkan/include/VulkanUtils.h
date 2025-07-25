#pragma once

#include <deque>
#include <vector>
#include <span>

#include "Common.h"
#include <vulkan/vulkan_core.h>
#include <vulkan/vk_enum_string_helper.h>

#define VK_CHECK(call) \
    if (auto vkResult = call; vkResult != VK_SUCCESS) { \
        errs("{} Failed. Vulkan Error {}", #call, string_VkResult(vkResult)); \
    }

class CVulkanUtils {
public:
	static VkImageSubresourceRange imageSubresourceRange(VkImageAspectFlags inAspectMask);
	static void copyImageToImage(VkCommandBuffer inCmd, VkImage inSource, VkImage inDestination, VkExtent2D inSrcSize, VkExtent2D inDstSize);
	static void transitionImage(VkCommandBuffer inCmd, VkImage inImage, VkImageLayout inCurrentLayout, VkImageLayout inNewLayout);
	static VkRenderingAttachmentInfo createAttachmentInfo(VkImageView view, VkClearValue* clear ,VkImageLayout layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
	static VkRenderingAttachmentInfo createDepthAttachmentInfo(VkImageView view, VkImageLayout layout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);
	static VkRenderingInfo createRenderingInfo(VkExtent2D inExtent, VkRenderingAttachmentInfo* inColorAttachement, VkRenderingAttachmentInfo* inDepthAttachement);
};

class CVulkanInfo {
public:
	static VkCommandPoolCreateInfo createCommandPoolInfo(uint32 inQueueFamilyIndex, VkCommandPoolCreateFlags inFlags = 0);
	static VkCommandBufferAllocateInfo createCommandAllocateInfo(VkCommandPool inPool, uint32 inCount = 1);
	static VkFenceCreateInfo createFenceInfo(VkFenceCreateFlags inFlags = 0);
	static VkSemaphoreCreateInfo createSemaphoreInfo(VkSemaphoreCreateFlags inFlags = 0);
	static VkCommandBufferBeginInfo createCommandBufferBeginInfo(VkCommandBufferUsageFlags inFlags = 0);
	static VkSemaphoreSubmitInfo submitSemaphoreInfo(VkPipelineStageFlags2 inStageMask, VkSemaphore inSemaphore);
	static VkCommandBufferSubmitInfo submitCommandBufferInfo(VkCommandBuffer inCmd);
	static VkSubmitInfo submitInfo(VkCommandBuffer* inCmd);
	static VkSubmitInfo2 submitInfo(const VkCommandBufferSubmitInfo* inCmd, const VkSemaphoreSubmitInfo* inSignalSemaphoreInfo, const VkSemaphoreSubmitInfo* inWaitSemaphoreInfo);
	static VkImageCreateInfo createImageInfo(VkFormat inFormat, VkImageUsageFlags inUsageFlags, VkExtent3D inExtent);
	static VkImageViewCreateInfo createImageViewInfo(VkFormat inFormat, VkImage inImage, VkImageAspectFlags inAspectFlags);
};

struct SDescriptorLayoutBuilder {

	std::vector<VkDescriptorSetLayoutBinding> bindings;

	void addBinding(uint32 inBinding, VkDescriptorType inDescriptorType);

	void clear();

	VkDescriptorSetLayout build(VkShaderStageFlags inShaderStages, void* pNext = nullptr, VkDescriptorSetLayoutCreateFlags inFlags = 0);
};

struct SDescriptorAllocator {

	struct PoolSizeRatio{
		VkDescriptorType mType;
		float mRatio;
	};

	VkDescriptorPool mPool;

	void init(uint32_t inInitialSets, std::span<PoolSizeRatio> inPoolRatios);

	void clear();

	void destroy();

	VkDescriptorSet allocate(VkDescriptorSetLayout inLayout, void* pNext = nullptr);

private:

	VkDescriptorPool getPool();
	static VkDescriptorPool createPool(uint32 inSetCount, std::span<PoolSizeRatio> inPoolRatios);

	std::vector<PoolSizeRatio> ratios;
	std::vector<VkDescriptorPool> fullPools;
	std::vector<VkDescriptorPool> readyPools;
	uint32 setsPerPool;
};

struct SDescriptorWriter {
	std::deque<VkDescriptorImageInfo> imageInfos;
	std::deque<VkDescriptorBufferInfo> bufferInfos;
	std::vector<VkWriteDescriptorSet> writes;

	void writeImage(uint32 inBinding, VkImageView inImage, VkSampler inSampler, VkImageLayout inLayout, VkDescriptorType inType);
	//TODO: write_sampler?
	/*
	The descriptor types that are allowed for a buffer are these.
	VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER
	VK_DESCRIPTOR_TYPE_STORAGE_BUFFER
	VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC
	VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC
	 */
	void writeBuffer(uint32 inBinding, VkBuffer inBuffer, size_t inSize, size_t inOffset, VkDescriptorType inType);

	void clear();
	void updateSet(VkDescriptorSet inSet);
};
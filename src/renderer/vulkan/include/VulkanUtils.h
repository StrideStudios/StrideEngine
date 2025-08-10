#pragma once

#include "Common.h"
#include <vulkan/vulkan_core.h>
#include <vulkan/vk_enum_string_helper.h>

#include "PipelineBuilder.h"
#include "ResourceManager.h"

#define VK_CHECK(call) \
	if (auto vkResult = call; vkResult != VK_SUCCESS) { \
		errs("{} Failed. Vulkan Error {}", #call, string_VkResult(vkResult)); \
	}

VkDevice getDevice();

// Create wrappers around Vulkan types that can be destroyed
#define CREATE_VK_TYPE(inName) \
	struct C##inName final : IDestroyable { \
		C##inName() = default; \
		C##inName(Vk##inName inType): m##inName(inType) {} \
		C##inName(const C##inName& in##inName): m##inName(in##inName.m##inName) {} \
		C##inName(Vk##inName##CreateInfo inCreateInfo) { \
			VK_CHECK(vkCreate##inName(getDevice(), &inCreateInfo, nullptr, &m##inName)); \
		} \
		virtual void destroy() override { vkDestroy##inName(getDevice(), m##inName, nullptr); } \
		Vk##inName operator->() const { return m##inName; } \
		operator Vk##inName() const { return m##inName; } \
		Vk##inName m##inName = nullptr; \
	}

CREATE_VK_TYPE(CommandPool);
CREATE_VK_TYPE(DescriptorPool);
CREATE_VK_TYPE(DescriptorSetLayout);
CREATE_VK_TYPE(Fence);
CREATE_VK_TYPE(ImageView);

struct CPipeline final : IDestroyable {
	CPipeline() = default;
	CPipeline(VkPipeline inType): mPipeline(inType) {}
	CPipeline(const CPipeline& inPipeline): mPipeline(inPipeline.mPipeline) {} \
	CPipeline(const CPipelineBuilder& inCreateInfo) {
		mPipeline = inCreateInfo.buildPipeline(getDevice());
	}
	virtual void destroy() override { vkDestroyPipeline(getDevice(), mPipeline, nullptr); }
	VkPipeline operator->() const { return mPipeline; }
	operator VkPipeline() const { return mPipeline; }
	VkPipeline mPipeline = nullptr;
};

CREATE_VK_TYPE(PipelineLayout);
CREATE_VK_TYPE(RenderPass);
CREATE_VK_TYPE(Sampler);
CREATE_VK_TYPE(Semaphore);
CREATE_VK_TYPE(ShaderModule);

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
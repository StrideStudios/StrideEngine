#pragma once

#include <list>
#include <vma/vk_mem_alloc.h>
#include <vulkan/vk_enum_string_helper.h>
#include <vulkan/vulkan_core.h>

#include "Renderer.h"
#include "VulkanUtils.h"
#include "VkBootstrap.h"

// Create wrappers around Vulkan types that can be destroyed
#define CREATE_VK_TYPE(inName) \
	struct C##inName final : IDestroyable { \
		C##inName() = default; \
		C##inName(Vk##inName inType): m##inName(inType) {} \
		C##inName(const C##inName& in##inName): m##inName(in##inName.m##inName) {} \
		C##inName(Vk##inName##CreateInfo inCreateInfo) { \
			VK_CHECK(vkCreate##inName(CRenderer::device(), &inCreateInfo, nullptr, &m##inName)); \
		} \
		virtual void destroy() override { vkDestroy##inName(CRenderer::device(), m##inName, nullptr); } \
		Vk##inName operator->() const { return m##inName; } \
		operator Vk##inName() const { return m##inName; } \
		Vk##inName m##inName = nullptr; \
	}

CREATE_VK_TYPE(CommandPool);
CREATE_VK_TYPE(DescriptorPool);
CREATE_VK_TYPE(DescriptorSetLayout);
CREATE_VK_TYPE(Fence);

struct CPipeline final : IDestroyable {
	CPipeline() = default;
	CPipeline(VkPipeline inType, VkPipelineLayout inLayout): mPipeline(inType), mLayout(inLayout) {}
	CPipeline(const CPipeline& inPipeline): mPipeline(inPipeline.mPipeline), mLayout(inPipeline.mLayout) {}
	virtual void destroy() override { vkDestroyPipeline(CRenderer::device(), mPipeline, nullptr); }
	VkPipeline operator->() const { return mPipeline; }
	operator VkPipeline() const { return mPipeline; }
	VkPipeline mPipeline = nullptr;
	VkPipelineLayout mLayout;
};

CREATE_VK_TYPE(PipelineLayout);
CREATE_VK_TYPE(RenderPass);
CREATE_VK_TYPE(Sampler);
CREATE_VK_TYPE(Semaphore);
CREATE_VK_TYPE(ShaderModule);

struct SImage_T : IDestroyable {
	std::string name = "Image";
	VkImage mImage = nullptr;
	VkImageView mImageView = nullptr;
	VmaAllocation mAllocation = nullptr;
	VkExtent3D mImageExtent;
	VkFormat mImageFormat = VK_FORMAT_UNDEFINED;
	uint32 mBindlessAddress = -1;
	VkImageLayout mLayout = VK_IMAGE_LAYOUT_UNDEFINED;

	virtual void destroy() override;
};

struct SBuffer_T : IDestroyable {
	VkBuffer buffer = nullptr;
	VmaAllocation allocation = nullptr;
	VmaAllocationInfo info = {};
	uint32 mBindlessAddress;

	virtual void destroy() override;

	no_discard void* GetMappedData() const;

	no_discard void mapData(void** data) const;

	no_discard void unMapData() const;
};

// Holds the resources needed for mesh rendering
struct SMeshBuffers_T : IDestroyable {
	SBuffer_T* indexBuffer = nullptr;
	SBuffer_T* vertexBuffer = nullptr;
};

// A command buffer that stores some temporary data to be removed/changed at end of rendering
// Ex: contains info for image transitions
struct SCommandBuffer {

	SCommandBuffer() = default;

	SCommandBuffer(const CCommandPool* inCmdPool) {
		VkCommandBufferAllocateInfo frameCmdAllocInfo = CVulkanInfo::createCommandAllocateInfo(*inCmdPool, 1);
		VK_CHECK(vkAllocateCommandBuffers(CRenderer::device(), &frameCmdAllocInfo, &cmd));
	}

	SCommandBuffer(VkCommandBuffer inCmd): cmd(inCmd) {}
	SCommandBuffer(const SCommandBuffer& inCmd): cmd(inCmd.cmd) {}

	//TODO: not all command buffers are one time submit, perhaps have a parent class
	void begin(const bool inReset = true) const {
		if (inReset) {
			VK_CHECK(vkResetCommandBuffer(cmd, 0));
		}

		VkCommandBufferBeginInfo cmdBeginInfo = CVulkanInfo::createCommandBufferBeginInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
		VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));
	}

	void end() {
		vkEndCommandBuffer(cmd);

		// Image transitions can no longer occur
		for (const auto image : imageTransitions) {
			image->mLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		}
		imageTransitions.clear();
	}

	VkCommandBuffer operator->() const { return cmd; }
	operator VkCommandBuffer() const { return cmd; }

	VkCommandBuffer cmd;
	std::forward_list<SImage_T*> imageTransitions;
};
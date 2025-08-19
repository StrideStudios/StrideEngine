#pragma once

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
CREATE_VK_TYPE(ImageView);

struct CPipeline final : IDestroyable {
	CPipeline() = default;
	CPipeline(VkPipeline inType, VkPipelineLayout inLayout): mPipeline(inType), mLayout(inLayout) {}
	CPipeline(const CPipeline& inPipeline): mPipeline(inPipeline.mPipeline), mLayout(inPipeline.mLayout) {} \
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

struct SImage_T final : IDestroyable {
	std::string name;
	VkImage mImage;
	VkImageView mImageView;
	VmaAllocation mAllocation;
	VkExtent3D mImageExtent;
	VkFormat mImageFormat;
	uint32 mBindlessAddress;

	virtual void destroy() override;
};

struct SBuffer_T final : IDestroyable {
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
struct SMeshBuffers_T final : IDestroyable {
	SBuffer_T* indexBuffer = nullptr;
	SBuffer_T* vertexBuffer = nullptr;
};
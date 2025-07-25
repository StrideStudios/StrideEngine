#pragma once

#include <functional>
#include <memory>
#include <vector>
#include "BasicTypes.h"
#include <vulkan/vulkan_core.h>

typedef std::unique_ptr<struct SImage_T> SImage;
typedef std::unique_ptr<struct SBuffer_T> SBuffer;
typedef std::unique_ptr<struct SMeshBuffers_T> SMeshBuffers;

// A class used to deallocate any resource that vulkan needs
// Meant to replace deletion queues that store full functions
// Instead, this class stores references, which are significantly smaller
class CResourceDeallocator {

	enum class Type : uint8 {
		BUFFER,
		COMMANDPOOL,
		DESCRIPTORPOOL,
		DESCRIPTORSETLAYOUT,
		FENCE,
		FRAMEBUFFER,
		IMAGE,
		IMAGEVIEW,
		PIPELINE,
		PIPELINELAYOUT,
		RENDERPASS,
		SAMPLER,
		SEMAPHORE,
		SHADERMODULE,
		SWAPCHAINKHR
	};

	// Easily create new objects that can be deallocated
#define FOR_EACH_TYPE(func) \
	func(VkCommandPool, CommandPool, COMMANDPOOL) \
	func(VkDescriptorPool, DescriptorPool, DESCRIPTORPOOL) \
	func(VkDescriptorSetLayout, DescriptorSetLayout, DESCRIPTORSETLAYOUT) \
	func(VkFence, Fence, FENCE) \
	func(VkImageView, ImageView, IMAGEVIEW) \
	func(VkPipeline, Pipeline, PIPELINE) \
	func(VkPipelineLayout, PipelineLayout, PIPELINELAYOUT) \
	func(VkRenderPass, RenderPass, RENDERPASS) \
	func(VkSampler, Sampler, SAMPLER) \
	func(VkSemaphore, Semaphore, SEMAPHORE) \
	func(VkShaderModule, ShaderModule, SHADERMODULE) \
	func(VkSwapchainKHR, SwapchainKHR, SWAPCHAINKHR)

	struct Resource {

#define CREATE_CONSTRUCTORS(inType, inName, inEnum) \
	Resource(inType inName) \
	: mType(Type::inEnum) { \
		mResource.inName = inName; \
	}

	FOR_EACH_TYPE(CREATE_CONSTRUCTORS)

#undef CREATE_CONSTRUCTORS

	// Buffers are allocated via VMA, and thus must be deallocated with it
	Resource(const SBuffer& buffer)
	: mType(Type::BUFFER) {
		mResource.buffer = buffer.get();
	}

	// Images are allocated via VMA, and thus must be deallocated with it
	Resource(const SImage& image)
	: mType(Type::IMAGE) {
		mResource.image = image.get();
	}

	void destroy() const;

	Type mType;

#define CREATE_UNION(inType, inName, inEnum) \
		inType inName;

	union {
		FOR_EACH_TYPE(CREATE_UNION)
		SBuffer_T* buffer;
		SImage_T* image;
	} mResource;

#undef CREATE_UNION

	};

public:

	CResourceDeallocator() = default;

	void push(const Resource inResource) {
		m_Resources.push_back(inResource);
	}

	void push(const std::function<void()>& func) {
		m_Functions.push_back(func);
	}

	void append(std::vector<Resource> inResources) {
		m_Resources.append_range(inResources);
	}

	void flush() {
		for (auto& resource : m_Resources) {
			resource.destroy();
		}
		m_Resources.clear();
		for (auto& function : m_Functions) {
			function();
		}
		m_Functions.clear();
	}

private:

	std::vector<Resource> m_Resources;

	std::vector<std::function<void()>> m_Functions;

};
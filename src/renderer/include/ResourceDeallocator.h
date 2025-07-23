#pragma once

#include <vector>
#include "BasicTypes.h"
#include <vulkan/vulkan_core.h>

#include "vk_mem_alloc.h"


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
	func(Buffer, BUFFER) \
	func(CommandPool, COMMANDPOOL) \
	func(DescriptorPool, DESCRIPTORPOOL) \
	func(DescriptorSetLayout, DESCRIPTORSETLAYOUT) \
	func(Fence, FENCE) \
	func(ImageView, IMAGEVIEW) \
	func(Pipeline, PIPELINE) \
	func(PipelineLayout, PIPELINELAYOUT) \
	func(RenderPass, RENDERPASS) \
	func(Sampler, SAMPLER) \
	func(Semaphore, SEMAPHORE) \
	func(ShaderModule, SHADERMODULE) \
	func(SwapchainKHR, SWAPCHAINKHR)

	struct Resource {

#define CREATE_CONSTRUCTORS(inName, inEnum) \
	Resource(Vk##inName* inName) \
	: mType(Type::inEnum) { \
		mResource.inName = inName; \
	}

	FOR_EACH_TYPE(CREATE_CONSTRUCTORS)

#undef CREATE_CONSTRUCTORS

	// Images are allocated via VMA, and thus must be deallocated with it
	Resource(VmaAllocator allocator, VkImage* image, VmaAllocation allocation)
	: mType(Type::IMAGE) {
		mResource.image = {allocator, image, allocation};
	}

	void destroy() const;

		Type mType;

#define CREATE_UNION(inName, inEnum) \
		Vk##inName* inName;

		union {
			FOR_EACH_TYPE(CREATE_UNION)
			struct {
				VmaAllocator allocator;
				VkImage* image;
				VmaAllocation allocation;
			} image;
		} mResource;

#undef CREATE_UNION

	};

public:

	CResourceDeallocator() = default;

	void push(const Resource inResource) {
		m_Resources.push_back(inResource);
	}

	void append(std::vector<Resource> inResources) {
		m_Resources.append_range(inResources);
	}

	void flush() {
		for (auto& resource : m_Resources) {
			resource.destroy();
		}
		m_Resources.clear();
	}

private:

	std::vector<Resource> m_Resources;

};
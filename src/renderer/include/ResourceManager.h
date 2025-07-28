#pragma once

#include <functional>
#include <memory>
#include <vma/vk_mem_alloc.h>
#include <vulkan/vulkan_core.h>
#include <vector>
#include <deque>
#include <span>

#include "Common.h"

class CVulkanDevice;

struct SImage_T {
	VkImage mImage;
	VkImageView mImageView;
	VmaAllocation mAllocation;
	VkExtent3D mImageExtent;
	VkFormat mImageFormat;
	uint32 mBindlessAddress;
};
typedef std::shared_ptr<SImage_T> SImage;

struct SBuffer_T {
	VkBuffer buffer = nullptr;
	VmaAllocation allocation = nullptr;
	VmaAllocationInfo info = {};

	no_discard void* GetMappedData() const;
};
typedef std::shared_ptr<SBuffer_T> SBuffer;

// Holds the resources needed for mesh rendering
struct SMeshBuffers_T {
	SBuffer indexBuffer = nullptr;
	SBuffer vertexBuffer = nullptr;
	VkDeviceAddress vertexBufferAddress;
};
typedef std::shared_ptr<SMeshBuffers_T> SMeshBuffers;

class IDestroyable {
public:

	virtual void destroy() {}
};

/*
 * Stores pointers to vulkan resources so they can be automatically deallocated when flush is called
 * Can also store IDestroyable pointers in which delete does not need to be called
 * This means initialization can be done in a local context to remove clutter
 */
constexpr static uint32 gMaxBindlessResources = 16536;
constexpr static uint32 gMaxSamplers = 32;

static uint32 gTextureBinding = 0;
static uint32 gSamplerBinding = 1;

class CResourceManager {

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

	// Since most Vulkan types have the same allocators and destroyers, they can be set here
#define FOR_EACH_TYPE(func) \
	func(VkCommandPool, CommandPool, COMMANDPOOL) \
	func(VkDescriptorPool, DescriptorPool, DESCRIPTORPOOL) \
	func(VkDescriptorSetLayout, DescriptorSetLayout, DESCRIPTORSETLAYOUT) \
	func(VkFence, Fence, FENCE) \
	func(VkImageView, ImageView, IMAGEVIEW) \
	func(VkPipelineLayout, PipelineLayout, PIPELINELAYOUT) \
	func(VkRenderPass, RenderPass, RENDERPASS) \
	func(VkSampler, Sampler, SAMPLER) \
	func(VkSemaphore, Semaphore, SEMAPHORE) \
	func(VkShaderModule, ShaderModule, SHADERMODULE)

	struct Resource {

#define CREATE_CONSTRUCTORS(inType, inName, inEnum) \
	Resource(const inType& inName) \
	: mType(Type::inEnum) { \
		ptr = (void*)inName; \
	}

	FOR_EACH_TYPE(CREATE_CONSTRUCTORS)
	CREATE_CONSTRUCTORS(VkPipeline, Pipeline, PIPELINE);

	// Buffers are allocated via VMA, and thus must be deallocated with it
	Resource(const SBuffer& buffer)
	: mType(Type::BUFFER) {
		mResource = buffer;
	}

	// Images are allocated via VMA, and thus must be deallocated with it
	Resource(const SImage& image)
	: mType(Type::IMAGE) {
		mResource = image;
	}

#undef CREATE_CONSTRUCTORS

	void destroy() const;

	Type mType;

	void* ptr = nullptr;

	// images and buffers need a shared resource so they don't get destroyed locally
	std::shared_ptr<void> mResource = nullptr;

	};

	constexpr static VmaAllocator& getAllocator() {
		static VmaAllocator allocator;
		return allocator;
	}

	constexpr static VkDescriptorPool& getBindlessDescriptorPool() {
		static VkDescriptorPool pool;
		return pool;
	}

public:

	constexpr static VkDescriptorSetLayout& getBindlessDescriptorSetLayout() {
		static VkDescriptorSetLayout layout;
		return layout;
	}

	constexpr static VkDescriptorSet& getBindlessDescriptorSet() {
		static VkDescriptorSet set;
		return set;
	}

	CResourceManager() = default;

	static void init();

	static void destroyAllocator();

	template <typename TType>
	requires std::is_convertible_v<TType, IDestroyable>
	TType* createDestroyable() {
		TType* destroyable = new TType();
		m_Destroyables.push_back(destroyable);
		return destroyable;
	}

	template <typename TType, typename... TArgs>
	requires std::is_convertible_v<TType, IDestroyable>
	TType* createDestroyable(TArgs&&... args) {
		TType* destroyable = new TType(args...);
		m_Destroyables.push_back(destroyable);
		return destroyable;
	}

	template <typename... TType>
	void push(const TType&... objects) {
		m_Resources.append_range(std::vector<Resource>{objects...});
	}

	void pushF(const std::function<void()>& func) {
		m_Functions.push_back(func);
	}

	void flush() {
		// Reverse iterate
		for (auto itr = m_Resources.rbegin(); itr != m_Resources.rend(); ++itr) {
			itr->destroy();
		}
		m_Resources.clear();
		for (const auto& destroyable : m_Destroyables) {
			destroyable->destroy();
			delete destroyable;
		}
		for (auto& function : m_Functions) {
			function();
		}
		m_Functions.clear();
	}

#define DECLARE_ALLOCATER(inType, inName, inEnum) \
	no_discard inType allocate##inName(const inType##CreateInfo& pCreateInfo, const VkAllocationCallbacks* pAllocator = nullptr);
#define DECLARE_DEALLOCATER(inType, inName, inEnum) \
	static void deallocate##inName(const inType& fence);

	FOR_EACH_TYPE(DECLARE_ALLOCATER)
	FOR_EACH_TYPE(DECLARE_DEALLOCATER)

	DECLARE_DEALLOCATER(VkPipeline, Pipeline, PIPELINE)

#undef CREATE_FUNCTIONS

	//
	// Command Buffer
	//

	// Command Buffer does not need to be deallocated
	no_discard static VkCommandBuffer allocateCommandBuffer(const VkCommandBufferAllocateInfo& pCreateInfo);

	//
	// Descriptors
	//

	//TODO: descriptors here
	static void bindDescriptorSets(VkCommandBuffer cmd, VkPipelineBindPoint inBindPoint, VkPipelineLayout inPipelineLayout, uint32 inFirstSet, uint32 inDescriptorSetCount, const VkDescriptorSet& inDescriptorSets);

	//
	// Pipelines
	//

	//TODO: have pipelines build by flags instead of a builder, for now this works fine
	no_discard VkPipeline allocatePipeline(const class CPipelineBuilder& inPipelineBuilder, const VkAllocationCallbacks* pAllocator = nullptr);

	static void bindPipeline(VkCommandBuffer cmd, VkPipelineBindPoint inBindPoint, VkPipeline inPipeline);

	//
	// Images
	//

	no_discard SImage allocateImage(const char* inDebugName, VkExtent3D inExtent, VkFormat inFormat, VkImageUsageFlags inFlags = 0, VkImageAspectFlags inViewFlags = 0, bool inMipmapped = false);

	no_discard SImage allocateImage(void* inData, const char* inDebugName, VkExtent3D inExtent, VkFormat inFormat, VkImageUsageFlags inFlags = 0, VkImageAspectFlags inViewFlags = 0, bool inMipmapped = false);

	static void deallocateImage(const SImage_T* inImage);

	static void deallocateImage(const SImage& inImage) { deallocateImage(inImage.get()); }


	//
	// Buffers
	//

	// VkBufferUsageFlags is VERY important (VMA_MEMORY_USAGE_GPU_ONLY, VMA_MEMORY_USAGE_CPU_ONLY, VMA_MEMORY_USAGE_CPU_TO_GPU, VMA_MEMORY_USAGE_GPU_TO_CPU)
	// VMA_MEMORY_USAGE_CPU_TO_GPU can be used for the small fast-access buffer on GPU that CPU can still write to (something important)
	no_discard SBuffer allocateBuffer(size_t allocSize, VmaMemoryUsage memoryUsage, VkBufferUsageFlags usage = 0);

	no_discard SMeshBuffers allocateMeshBuffer(size_t indicesSize, size_t verticesSize);

	static void deallocateBuffer(const SBuffer_T* inBuffer);

	static void deallocateBuffer(const SBuffer& inImage) { deallocateBuffer(inImage.get()); }

private:

	std::vector<IDestroyable*> m_Destroyables;

	std::vector<Resource> m_Resources;

	std::vector<std::function<void()>> m_Functions;

};

#pragma once

#include <functional>
#include <memory>
#include <vma/vk_mem_alloc.h>
#include <vulkan/vulkan_core.h>
#include <vector>
#include <deque>
#include <span>

#include "Common.h"
#include "VulkanUtils.h"
#include "ResourceManager.h"

class CVulkanDevice;

struct SImage_T : IDestroyable {
	std::string name;
	VkImage mImage;
	VkImageView mImageView;
	VmaAllocation mAllocation;
	VkExtent3D mImageExtent;
	VkFormat mImageFormat;
	uint32 mBindlessAddress;

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
	SBuffer_T* instanceBuffer = nullptr;
};

enum class EBlendMode : uint8 {
	NONE,
	ADDITIVE,
	ALPHA_BLEND
};

enum class EDepthTestMode : uint8 {
	NORMAL,
	BEHIND,
	FRONT
};

struct SPipelineCreateInfo {
	VkShaderModule vertexModule;
	VkShaderModule fragmentModule;
	VkPrimitiveTopology mTopology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	VkPolygonMode mPolygonMode = VK_POLYGON_MODE_FILL;
	VkCullModeFlags mCullMode = VK_CULL_MODE_FRONT_BIT;
	VkFrontFace mFrontFace = VK_FRONT_FACE_CLOCKWISE;
	bool mUseMultisampling = false;
	EBlendMode mBlendMode = EBlendMode::NONE;
	EDepthTestMode mDepthTestMode = EDepthTestMode::NORMAL;
	VkFormat mColorFormat;
	VkFormat mDepthFormat;
};

class CVertexAttributeArchive {

public:

	void createBinding(VkVertexInputRate InputRate) {
		m_Formats.emplace(InputRate, std::vector<VkFormat>());
	}

	VkPipelineVertexInputStateCreateInfo get() {
		m_Bindings.clear();
		m_Attributes.clear();

		uint32 location = 0;
		uint32 binding = 0;
		for (auto& [InputRate, formats] : m_Formats) {
			uint32 stride = 0;
			for (uint32 current = 0; current < formats.size(); ++current, ++location) {
				const auto& format = formats[current];
				m_Attributes.push_back(VkVertexInputAttributeDescription{
					location,
					binding,
					format,
					stride
				});
				stride += getVkFormatSize(format);
			}
			m_Bindings.push_back({
				.binding = binding,
				.stride = stride,
				.inputRate = InputRate
			});
			binding++;
		}

		return
		{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
			.vertexBindingDescriptionCount = static_cast<uint32>(m_Bindings.size()),
			.pVertexBindingDescriptions = m_Bindings.data(),
			.vertexAttributeDescriptionCount = static_cast<uint32>(m_Attributes.size()),
			.pVertexAttributeDescriptions = m_Attributes.data()
		};
	}

	friend CVertexAttributeArchive& operator<<(CVertexAttributeArchive& inArchive, VkFormat inFormat) {
		inArchive.m_Formats.rbegin()->second.push_back(inFormat);
		return inArchive;
	}

private:

	// Stored here because the data needs to last the lifetime of this object
	std::vector<VkVertexInputBindingDescription> m_Bindings;
	std::vector<VkVertexInputAttributeDescription> m_Attributes;

	std::map<VkVertexInputRate, std::vector<VkFormat>> m_Formats;
};

// More than 65535 textures should not be needed, but more than 255 might be.
constexpr static uint32 gMaxTextures = std::numeric_limits<uint16>::max();
// There are very few types of samplers, so only 32 are needed
constexpr static uint32 gMaxSamplers = 32;
// Uniform buffers tend to be fast to access but very small, max of 255
// OpenGL spec states that uniform buffers guarantee up to 16 KB
// (Subtracted by gMaxSamplers to keep alignment)
constexpr static uint32 gMaxUniformBuffers = std::numeric_limits<uint8>::max() - gMaxSamplers;
// Shader Storage Buffers tend to be slower but larger
// OpenGL spec states that SSBOs guarantee up to 128 MB, but can be larger
constexpr static uint32 gMaxStorageBuffers = std::numeric_limits<uint16>::max();

static uint32 gTextureBinding = 0;
static uint32 gSamplerBinding = 1;
static uint32 gUBOBinding = 2;
static uint32 gSSBOBinding = 3;

/*
 * Specific version of CResourceManager for vulkan resources
 */
class CVulkanResourceManager : public CResourceManager {

	static VkDevice getDevice();

	friend SBuffer_T;
	friend SImage_T;

	constexpr static VmaAllocator& getAllocator() {
		static VmaAllocator allocator;
		return allocator;
	}

	constexpr static CDescriptorPool*& getBindlessDescriptorPool() {
		static CDescriptorPool* pool;
		return pool;
	}

public:

	constexpr static CPipelineLayout*& getBasicPipelineLayout() {
		static CPipelineLayout* layout;
		return layout;
	}

	constexpr static CDescriptorSetLayout*& getBindlessDescriptorSetLayout() {
		static CDescriptorSetLayout* layout;
		return layout;
	}

	constexpr static VkDescriptorSet& getBindlessDescriptorSet() {
		static VkDescriptorSet set;
		return set;
	}

	CVulkanResourceManager() = default;

	static void init();

	static void destroy();

	/*template <typename TType, typename TCreator>
	requires std::constructible_from<Resource<TType>, TCreator>
	TType push(const TCreator& object) {
		Resource<TType>* outType = new Resource<TType>(object);
		m_Destroyables.push_back(outType);
		return outType->mResource;
	}*/

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

	no_discard CPipeline* allocatePipeline(const SPipelineCreateInfo& inCreateInfo, CVertexAttributeArchive& inAttributes, CPipelineLayout* inLayout);

	static void bindPipeline(VkCommandBuffer cmd, VkPipelineBindPoint inBindPoint, VkPipeline inPipeline);

	//
	// Buffers
	//

	// VkBufferUsageFlags is VERY important (VMA_MEMORY_USAGE_GPU_ONLY, VMA_MEMORY_USAGE_CPU_ONLY, VMA_MEMORY_USAGE_CPU_TO_GPU, VMA_MEMORY_USAGE_GPU_TO_CPU)
	// VMA_MEMORY_USAGE_CPU_TO_GPU can be used for the small fast-access buffer on GPU that CPU can still write to (something important)
	no_discard SBuffer_T* allocateBuffer(size_t allocSize, VmaMemoryUsage memoryUsage, VkBufferUsageFlags usage = 0);

	no_discard SBuffer_T* allocateGlobalBuffer(size_t allocSize, VmaMemoryUsage memoryUsage, VkBufferUsageFlags usage = 0);

	static void updateGlobalBuffer(const SBuffer_T* buffer);

	no_discard SMeshBuffers_T allocateMeshBuffer(size_t indicesSize, size_t verticesSize);

	//
	// Images
	// Since bindless images are used, it is unnecessary to keep the result
	//

	SImage_T* allocateImage(const std::string& inDebugName, VkExtent3D inExtent, VkFormat inFormat, VkImageUsageFlags inFlags = 0, VkImageAspectFlags inViewFlags = 0, bool inMipmapped = false);

	SImage_T* allocateImage(const std::string& inDebugName, VkExtent3D inExtent, VkFormat inFormat, VkImageUsageFlags inFlags = 0, VkImageAspectFlags inViewFlags = 0, uint32 NumMips = 1);

	SImage_T* allocateImage(void* inData, const uint32& size, const std::string& inDebugName, VkExtent3D inExtent, VkFormat inFormat, VkImageUsageFlags inFlags = 0, VkImageAspectFlags inViewFlags = 0, bool inMipmapped = false);

};

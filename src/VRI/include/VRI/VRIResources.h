#pragma once

#include <forward_list>
#include <vma/vk_mem_alloc.h>
#include <vulkan/vk_enum_string_helper.h>
#include <vulkan/vulkan_core.h>

#include "VkBootstrap.h"
#include "VRI.h"
//#include "VRI/VRIUtils.h"

#include "basic/core/Common.h"
#include "sstl/Vector.h"

#define VK_CHECK(call) \
	if (auto vkResult = call; vkResult != VK_SUCCESS) { \
		errs("{} Failed. Vulkan Error {}", #call, string_VkResult(vkResult)); \
	}

enum class EShaderStage : uint8 {
	VERTEX,
	FRAGMENT,
	COMPUTE
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
	float mLineWidth = 1.f;
	VkCullModeFlags mCullMode = VK_CULL_MODE_FRONT_BIT;
	VkFrontFace mFrontFace = VK_FRONT_FACE_CLOCKWISE;
	bool mUseMultisampling = false;
	EBlendMode mBlendMode = EBlendMode::NONE;
	EDepthTestMode mDepthTestMode = EDepthTestMode::NORMAL;
	VkFormat mColorFormat;
	VkFormat mDepthFormat;
};

class CVertexAttributeArchive {

	struct VertexAttributeFormat {
		VkVertexInputRate inputRate;
		TVector<VkFormat> formats;
	};

public:

	void createBinding(const VkVertexInputRate InputRate) {
		m_Formats.push(VertexAttributeFormat{InputRate, TVector<VkFormat>{}});
	}

	VkPipelineVertexInputStateCreateInfo get() {
		m_Bindings.clear();
		m_Attributes.clear();

		uint32 location = 0;
		uint32 binding = 0;
		m_Formats.forEach([&](size_t, const VertexAttributeFormat& formats) {
			uint32 stride = 0;
			for (uint32 current = 0; current < formats.formats.getSize(); ++current, ++location) {
				const auto& format = formats.formats[current];
				m_Attributes.push(VkVertexInputAttributeDescription{
					location,
					binding,
					format,
					stride
				});
				//stride += getVkFormatSize(format);
			}
			m_Bindings.push({
				.binding = binding,
				.stride = stride,
				.inputRate = formats.inputRate
			});
			binding++;
		});

		return
		{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
			.vertexBindingDescriptionCount = static_cast<uint32>(m_Bindings.getSize()),
			.pVertexBindingDescriptions = m_Bindings.data(),
			.vertexAttributeDescriptionCount = static_cast<uint32>(m_Attributes.getSize()),
			.pVertexAttributeDescriptions = m_Attributes.data()
		};
	}

	friend CVertexAttributeArchive& operator<<(CVertexAttributeArchive& inArchive, VkFormat inFormat) {
		inArchive.m_Formats.top().formats.push(inFormat);
		return inArchive;
	}

private:

	// Stored here because the data needs to last the lifetime of this object
	TVector<VkVertexInputBindingDescription> m_Bindings;
	TVector<VkVertexInputAttributeDescription> m_Attributes;

	TVector<VertexAttributeFormat> m_Formats;
};

// Create wrappers around Vulkan types that can be destroyed
#define CREATE_VK_TYPE(inName) \
	struct C##inName  { \
		C##inName() = default; \
		C##inName(Vk##inName inType): m##inName(inType) {} \
		C##inName(const C##inName& in##inName): m##inName(in##inName.m##inName) {} \
		C##inName(Vk##inName##CreateInfo inCreateInfo) { \
			VK_CHECK(vkCreate##inName(CVRI::get()->getDevice()->device, &inCreateInfo, nullptr, &m##inName)); \
		} \
		Vk##inName get() { return m##inName; } \
		void destroy() { vkDestroy##inName(CVRI::get()->getDevice()->device, m##inName, nullptr); } \
		Vk##inName operator->() const { return m##inName; } \
		operator Vk##inName() const { return m##inName; } \
		Vk##inName m##inName = nullptr; \
	}

CREATE_VK_TYPE(CommandPool);
CREATE_VK_TYPE(DescriptorPool);
CREATE_VK_TYPE(DescriptorSetLayout);
CREATE_VK_TYPE(Fence);
CREATE_VK_TYPE(PipelineLayout);

struct CPipeline  {

	CPipeline() = default;
	EXPORT CPipeline(const SPipelineCreateInfo& inCreateInfo, CVertexAttributeArchive& inAttributes, const TUnique<CPipelineLayout>& inLayout);

	EXPORT void bind(VkCommandBuffer cmd, const VkPipelineBindPoint inBindPoint) const;

	EXPORT void destroy();

	VkPipeline operator->() const { return mPipeline; }

	operator VkPipeline() const { return mPipeline; }

	VkPipeline mPipeline = nullptr;

	CPipelineLayout* mLayout;

};

CREATE_VK_TYPE(RenderPass);
CREATE_VK_TYPE(Sampler);
CREATE_VK_TYPE(Semaphore);
CREATE_VK_TYPE(ShaderModule); //TODO: remove

struct CDescriptorSet  {

	CDescriptorSet() = default;

	CDescriptorSet(const VkDescriptorSet inType): mDescriptorSet(inType) {}

	CDescriptorSet(const CDescriptorSet& inDescriptorSet): mDescriptorSet(inDescriptorSet.mDescriptorSet) {}

	CDescriptorSet(const VkDescriptorSetAllocateInfo& inCreateInfo) {
		VK_CHECK(vkAllocateDescriptorSets(CVRI::get()->getDevice()->device, &inCreateInfo, &mDescriptorSet));
	}

	void bind(VkCommandBuffer cmd, VkPipelineBindPoint inBindPoint, VkPipelineLayout inPipelineLayout, uint32 inFirstSet, uint32 inDescriptorSetCount) const {
		vkCmdBindDescriptorSets(cmd, inBindPoint,inPipelineLayout, inFirstSet, inDescriptorSetCount, &mDescriptorSet, 0, nullptr);
	}

	VkDescriptorSet operator->() const { return mDescriptorSet; }
	operator VkDescriptorSet() const { return mDescriptorSet; }

	VkDescriptorSet mDescriptorSet = nullptr;
};

struct SShader  {

	SShader() = default;
	EXPORT SShader(const char* inFilePath);

	EXPORT void destroy();

	std::string mFileName = "";
	VkShaderModule mModule = nullptr;
	EShaderStage mStage = EShaderStage::VERTEX;
	std::string mShaderCode = "";
	TVector<uint32> mCompiledShader{};

private:

	uint32 compile();

	bool loadShader(const char* inFileName, uint32 Hash);

	bool saveShader(const char* inFileName, uint32 Hash) const;
};

enum class EAttachmentType : uint8 {
	COLOR,
	DEPTH,
	STENCIL
};

struct SRenderAttachment {
	VkAttachmentLoadOp mLoadOp = VK_ATTACHMENT_LOAD_OP_NONE;
	VkAttachmentStoreOp mStoreOp = VK_ATTACHMENT_STORE_OP_NONE;
	EAttachmentType mType = EAttachmentType::COLOR;
	Vector4f mClearValue = Vector4f{0.f};

	constexpr static SRenderAttachment defaultColor() {
		return {
			.mLoadOp = VK_ATTACHMENT_LOAD_OP_LOAD,
			.mStoreOp = VK_ATTACHMENT_STORE_OP_STORE,
			.mType = EAttachmentType::COLOR
		};
	}

	constexpr static SRenderAttachment defaultDepth() {
		return {
			.mLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
			.mStoreOp = VK_ATTACHMENT_STORE_OP_STORE,
			.mType = EAttachmentType::DEPTH
		};
	}

	constexpr static SRenderAttachment defaultStencil() {
		return {
			.mLoadOp = VK_ATTACHMENT_LOAD_OP_NONE,
			.mStoreOp = VK_ATTACHMENT_STORE_OP_NONE,
			.mType = EAttachmentType::STENCIL
		};
	}

	EXPORT VkRenderingAttachmentInfo get(const struct SVRIImage* inImage) const;

	bool operator==(const SRenderAttachment& inOther) const {
		return mLoadOp == inOther.mLoadOp && mStoreOp == inOther.mStoreOp && mClearValue == inOther.mClearValue;
	}
};


struct SVRIBuffer  {

	EXPORT SVRIBuffer(VmaAllocator inAllocator, size_t inBufferSize, VmaMemoryUsage inMemoryUsage, VkBufferUsageFlags inBufferUsage = 0);

	EXPORT void makeGlobal(); //TODO: alternate way of doing this
	EXPORT void updateGlobal() const; // TODO: not global but instead 'dynamic' (address should be separate?)

	EXPORT void destroy();

	no_discard EXPORT void* getMappedData() const;

	EXPORT void mapData(void** data) const;

	EXPORT void unMapData() const;

	bool isAllocated() const {
		return allocation != nullptr;
	}

	template <typename... TArgs>
	void push(const void* src, const size_t size, TArgs... args) {
		memcpy(getMappedData(), src, size);
		push(size, args...);
	}

	VkBuffer buffer = nullptr;

	VmaAllocator allocator = nullptr;
	VmaAllocation allocation = nullptr;
	VmaAllocationInfo info = {};
	size_t size;

	uint32 mBindlessAddress = 0;

private:

	template <typename... TArgs>
	void push(const size_t offset = 0) {}

	template <typename... TArgs>
	void push(const size_t offset, const void* src, const size_t size, TArgs... args) {
		memcpy(static_cast<char*>(getMappedData()) + offset, src, size);
		push(offset + size, args...);
	}
};

// Holds the resources needed for mesh rendering
struct SVRIMeshBuffer  {

	SVRIMeshBuffer() = default;
	EXPORT SVRIMeshBuffer(VmaAllocator allocator, size_t indicesSize, size_t verticesSize);

	TUnique<SVRIBuffer> indexBuffer = nullptr;
	TUnique<SVRIBuffer> vertexBuffer = nullptr;
};

struct SVRIImage {

	SVRIImage() = default;
	EXPORT SVRIImage(VmaAllocator allocator, const std::string& inDebugName, VkExtent3D inExtent, VkFormat inFormat, VkImageUsageFlags inFlags = 0, VkImageAspectFlags inViewFlags = 0, uint32 inNumMips = 1);

	VkExtent3D getExtent() const { return mImageInfo.extent; }

	VkFormat getFormat() const { return mImageInfo.format; }

	bool isMipmapped() const { return mImageInfo.mipLevels > 1; }

	EXPORT void destroy(VmaAllocator allocator);

	EXPORT void push(VmaAllocator allocator, const TFrail<class CVRICommands>& cmd, const void* inData, const uint32& inSize);

	std::string mName = "Image";

	VkImage mImage = nullptr;
	VkImageCreateInfo mImageInfo;

	VkImageView mImageView = nullptr;
	VkImageViewCreateInfo mImageViewInfo;

	VmaAllocation mAllocation = nullptr;
	uint32 mBindlessAddress = -1;

	VkImageLayout mLayout = VK_IMAGE_LAYOUT_UNDEFINED;

};
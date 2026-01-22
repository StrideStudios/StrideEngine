#pragma once

#include <forward_list>
#include <vma/vk_mem_alloc.h>
#include <vulkan/vk_enum_string_helper.h>
#include <vulkan/vulkan_core.h>

#include "VkBootstrap.h"
#include "VRI.h"

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

struct SVRIResource {

	EXPORT SVRIResource();
	virtual ~SVRIResource() = default;

	// No copying vulkan resources, since most need to be deleted, and if they are copied, could be deleted more than once
	SVRIResource(const SVRIResource&) = delete;
	SVRIResource& operator=(const SVRIResource&) = delete;

	virtual std::function<void()> getDestroyer() = 0;

	EXPORT void release();
};

// Create wrappers around Vulkan types that can be destroyed
#define CREATE_VK_TYPE(n) \
	struct C##n : SVRIResource { \
		C##n() = default; \
		C##n(Vk##n inType): m##n(inType) {} \
		C##n(const C##n& in##n): m##n(in##n.m##n) {} \
		C##n(Vk##n##CreateInfo inCreateInfo) { \
			VK_CHECK(vkCreate##n(CVRI::get()->getDevice()->device, &inCreateInfo, nullptr, &m##n)); \
		} \
		Vk##n get() { return m##n; } \
		virtual std::function<void()> getDestroyer() override { return [n = m##n] { vkDestroy##n(CVRI::get()->getDevice()->device, n, nullptr); }; } \
		Vk##n operator->() const { return m##n; } \
		operator Vk##n() const { return m##n; } \
		Vk##n m##n = nullptr; \
	}

CREATE_VK_TYPE(CommandPool);
CREATE_VK_TYPE(DescriptorPool);
CREATE_VK_TYPE(DescriptorSetLayout);
CREATE_VK_TYPE(Fence);
CREATE_VK_TYPE(PipelineLayout);

//TODO: rename
struct SPipeline : SVRIResource {

	SPipeline() = default;
	EXPORT SPipeline(const SPipelineCreateInfo& inCreateInfo, CVertexAttributeArchive& inAttributes, const TUnique<CPipelineLayout>& inLayout);

	EXPORT void bind(VkCommandBuffer cmd, const VkPipelineBindPoint inBindPoint) const;

	EXPORT virtual std::function<void()> getDestroyer() override;

	VkPipeline operator->() const { return mPipeline; }

	operator VkPipeline() const { return mPipeline; }

	VkPipeline mPipeline = nullptr;

	CPipelineLayout* mLayout;
};

CREATE_VK_TYPE(RenderPass);
CREATE_VK_TYPE(Sampler);
CREATE_VK_TYPE(Semaphore);
CREATE_VK_TYPE(ShaderModule); //TODO: remove

#undef CREATE_VK_TYPE

struct CDescriptorSet : SVRIResource {

	CDescriptorSet() = default;

	CDescriptorSet(const VkDescriptorSet inType): mDescriptorSet(inType) {}

	CDescriptorSet(const CDescriptorSet& inDescriptorSet): mDescriptorSet(inDescriptorSet.mDescriptorSet) {}

	CDescriptorSet(const VkDescriptorSetAllocateInfo& inCreateInfo) {
		VK_CHECK(vkAllocateDescriptorSets(CVRI::get()->getDevice()->device, &inCreateInfo, &mDescriptorSet));
	}

	virtual std::function<void()> getDestroyer() override { return []{}; }

	void bind(VkCommandBuffer cmd, VkPipelineBindPoint inBindPoint, VkPipelineLayout inPipelineLayout, uint32 inFirstSet, uint32 inDescriptorSetCount) const {
		vkCmdBindDescriptorSets(cmd, inBindPoint,inPipelineLayout, inFirstSet, inDescriptorSetCount, &mDescriptorSet, 0, nullptr);
	}

	VkDescriptorSet operator->() const { return mDescriptorSet; }
	operator VkDescriptorSet() const { return mDescriptorSet; }

	VkDescriptorSet mDescriptorSet = nullptr;
};

struct SShader : SVRIResource {

	SShader() = default;
	EXPORT SShader(const char* inFilePath);

	EXPORT virtual std::function<void()> getDestroyer() override;

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


struct SVRIBuffer : SVRIResource {

	EXPORT SVRIBuffer(size_t inBufferSize, VmaMemoryUsage inMemoryUsage, VkBufferUsageFlags inBufferUsage = 0);

	EXPORT void makeGlobal(); //TODO: alternate way of doing this
	EXPORT void updateGlobal() const; // TODO: not global but instead 'dynamic' (address should be separate?)

	EXPORT virtual std::function<void()> getDestroyer() override;

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
struct SVRIMeshBuffer {

	SVRIMeshBuffer() = default;
	EXPORT SVRIMeshBuffer(size_t indicesSize, size_t verticesSize);

	TUnique<SVRIBuffer> indexBuffer = nullptr;
	TUnique<SVRIBuffer> vertexBuffer = nullptr;
};

struct SVRIImage : SVRIResource {

	SVRIImage() = default;
	EXPORT SVRIImage(const std::string& inDebugName, VkExtent3D inExtent, VkFormat inFormat, VkImageUsageFlags inFlags = 0, VkImageAspectFlags inViewFlags = 0, uint32 inNumMips = 1);

	EXPORT virtual std::function<void()> getDestroyer() override;

	VkExtent3D getExtent() const { return mImageInfo.extent; }

	VkFormat getFormat() const { return mImageInfo.format; }

	bool isMipmapped() const { return mImageInfo.mipLevels > 1; }

	EXPORT void push(const TFrail<class CVRICommands>& cmd, const void* inData, const uint32& inSize);

	std::string mName = "Image";

	VkImage mImage = nullptr;
	VkImageCreateInfo mImageInfo;

	VkImageView mImageView = nullptr;
	VkImageViewCreateInfo mImageViewInfo;

	VmaAllocation mAllocation = nullptr;
	uint32 mBindlessAddress = -1;

	VkImageLayout mLayout = VK_IMAGE_LAYOUT_UNDEFINED;

};
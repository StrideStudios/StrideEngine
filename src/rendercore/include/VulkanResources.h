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

enum class EAttachmentType : uint8 {
	COLOR,
	DEPTH,
	STENCIL
};

struct EXPORT SRenderAttachment {
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

	VkRenderingAttachmentInfo get(const SImage_T* inImage) const;

	bool operator==(const SRenderAttachment& inOther) const {
		return mLoadOp == inOther.mLoadOp && mStoreOp == inOther.mStoreOp && mClearValue == inOther.mClearValue;
	}
};

struct SImage_T : IDestroyable {
	std::string name = "Image";
	VkImage mImage = nullptr;
	VkImageView mImageView = nullptr;
	VmaAllocation mAllocation = nullptr;
	VkExtent3D mImageExtent;
	VkFormat mImageFormat = VK_FORMAT_UNDEFINED;
	uint32 mBindlessAddress = -1;
	VkImageLayout mLayout = VK_IMAGE_LAYOUT_UNDEFINED;

	EXPORT virtual void destroy() override;
};

struct SBuffer_T : IDestroyable {
	VkBuffer buffer = nullptr;
	VmaAllocation allocation = nullptr;
	VmaAllocationInfo info = {};
	uint32 mBindlessAddress;

	EXPORT virtual void destroy() override;

	no_discard EXPORT void* GetMappedData() const;

	EXPORT void mapData(void** data) const;

	EXPORT void unMapData() const;
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

struct SInstance {
	Matrix4f Transform = Matrix4f(1.f);

	friend CArchive& operator<<(CArchive& inArchive, const SInstance& inInstance) {
		inArchive << inInstance.Transform;
		return inArchive;
	}

	friend CArchive& operator>>(CArchive& inArchive, SInstance& inInstance) {
		inArchive >> inInstance.Transform;
		return inArchive;
	}
};

struct EXPORT SInstancer {

	SInstancer(uint32 initialSize = 0);

	~SInstancer();

	void append(const std::vector<SInstance>& inInstances) {
		instances.append_range(inInstances);
		setDirty();
	}

	uint32 push(const SInstance& inInstance) {
		instances.push_back(inInstance);
		setDirty();
		return static_cast<uint32>(instances.size()) - 1;
	}

	SInstance remove(const uint32 inInstance) {
		const auto& instance = instances.erase(instances.begin() + inInstance);
		setDirty();
		return *instance;
	}

	void flush() {
		instances.clear();
		setDirty();
	}

	void reallocate(const Matrix4f& parentMatrix = Matrix4f(1.f));

	SBuffer_T* get(const Matrix4f& parentMatrix = Matrix4f(1.f)) {
		if (isDirty()) {
			mIsDirty = false;
			reallocate(parentMatrix);
		}
		return instanceBuffer;
	}

	bool isDirty() const {
		return mIsDirty;
	}

	void setDirty() {
		mIsDirty = true;
	}

	friend CArchive& operator<<(CArchive& inArchive, const SInstancer& inInstancer) {
		inArchive << inInstancer.instances;
		return inArchive;
	}

	friend CArchive& operator>>(CArchive& inArchive, SInstancer& inInstancer) {
		inArchive >> inInstancer.instances;
		return inArchive;
	}

	bool mIsDirty = true;

	std::vector<SInstance> instances;

private:

	SBuffer_T* instanceBuffer = nullptr;

};
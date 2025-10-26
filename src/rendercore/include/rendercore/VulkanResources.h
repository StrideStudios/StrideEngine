#pragma once

#include <forward_list>
#include <vma/vk_mem_alloc.h>
#include <vulkan/vk_enum_string_helper.h>
#include <vulkan/vulkan_core.h>

#include "rendercore/Renderer.h"
#include "rendercore/VulkanUtils.h"
#include "basic/core/Common.h"

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

// Create wrappers around Vulkan types that can be destroyed
#define CREATE_VK_TYPE(inName) \
	struct C##inName final : SBase, IDestroyable { \
		C##inName() = default; \
		C##inName(Vk##inName inType): m##inName(inType) {} \
		C##inName(const C##inName& in##inName): m##inName(in##inName.m##inName) {} \
		C##inName(Vk##inName##CreateInfo inCreateInfo) { \
			VK_CHECK(vkCreate##inName(CRenderer::vkDevice(), &inCreateInfo, nullptr, &m##inName)); \
		} \
		virtual void destroy() override { vkDestroy##inName(CRenderer::vkDevice(), m##inName, nullptr); } \
		Vk##inName operator->() const { return m##inName; } \
		operator Vk##inName() const { return m##inName; } \
		Vk##inName m##inName = nullptr; \
	}

CREATE_VK_TYPE(CommandPool);
CREATE_VK_TYPE(DescriptorPool);
CREATE_VK_TYPE(DescriptorSetLayout);
CREATE_VK_TYPE(Fence);
CREATE_VK_TYPE(PipelineLayout);

struct CPipeline final : SBase, IDestroyable {

	CPipeline() = default;
	EXPORT CPipeline(const SPipelineCreateInfo& inCreateInfo, CVertexAttributeArchive& inAttributes, CPipelineLayout* inLayout);

	EXPORT void bind(VkCommandBuffer cmd, const VkPipelineBindPoint inBindPoint) const;

	virtual void destroy() override { vkDestroyPipeline(CRenderer::vkDevice(), mPipeline, nullptr); }

	VkPipeline operator->() const { return mPipeline; }

	operator VkPipeline() const { return mPipeline; }

	VkPipeline mPipeline = nullptr;

	CPipelineLayout* mLayout;

};

CREATE_VK_TYPE(RenderPass);
CREATE_VK_TYPE(Sampler);
CREATE_VK_TYPE(Semaphore);
CREATE_VK_TYPE(ShaderModule); //TODO: remove

struct CDescriptorSet final : SBase {

	CDescriptorSet() = default;

	CDescriptorSet(const VkDescriptorSet inType): mDescriptorSet(inType) {}

	CDescriptorSet(const CDescriptorSet& inDescriptorSet): mDescriptorSet(inDescriptorSet.mDescriptorSet) {}

	CDescriptorSet(const VkDescriptorSetAllocateInfo& inCreateInfo) {
		VK_CHECK(vkAllocateDescriptorSets(CRenderer::vkDevice(), &inCreateInfo, &mDescriptorSet));
	}

	void bind(VkCommandBuffer cmd, VkPipelineBindPoint inBindPoint, VkPipelineLayout inPipelineLayout, uint32 inFirstSet, uint32 inDescriptorSetCount) const {
		vkCmdBindDescriptorSets(cmd, inBindPoint,inPipelineLayout, inFirstSet, inDescriptorSetCount, &mDescriptorSet, 0, nullptr);
	}

	VkDescriptorSet operator->() const { return mDescriptorSet; }
	operator VkDescriptorSet() const { return mDescriptorSet; }

	VkDescriptorSet mDescriptorSet = nullptr;
};

struct SShader : SBase, IDestroyable {

	SShader() = default;
	EXPORT SShader(const char* inFilePath);

	virtual void destroy() override { vkDestroyShaderModule(CRenderer::vkDevice(), mModule, nullptr); }

	std::string mFileName = "";
	VkShaderModule mModule = nullptr;
	EShaderStage mStage = EShaderStage::VERTEX;
	std::string mShaderCode = "";
	std::vector<uint32> mCompiledShader{};

private:

	uint32 compile();

	bool loadShader(VkDevice inDevice, const char* inFileName, uint32 Hash);

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

	EXPORT VkRenderingAttachmentInfo get(const SImage_T* inImage) const;

	bool operator==(const SRenderAttachment& inOther) const {
		return mLoadOp == inOther.mLoadOp && mStoreOp == inOther.mStoreOp && mClearValue == inOther.mClearValue;
	}
};

struct SImage_T : SBase, IDestroyable {

	SImage_T() = default;
	EXPORT SImage_T(const std::string& inDebugName, VkExtent3D inExtent, VkFormat inFormat, VkImageUsageFlags inFlags = 0, VkImageAspectFlags inViewFlags = 0, uint32 inNumMips = 1);

	VkExtent3D getExtent() const { return mImageInfo.extent; }

	VkFormat getFormat() const { return mImageInfo.format; }

	bool isMipmapped() const { return mImageInfo.mipLevels > 1; }

	EXPORT virtual void destroy() override;

	EXPORT void push(const void* inData, const uint32& inSize);

	std::string mName = "Image";

	VkImage mImage = nullptr;
	VkImageCreateInfo mImageInfo;

	VkImageView mImageView = nullptr;
	VkImageViewCreateInfo mImageViewInfo;

	VmaAllocation mAllocation = nullptr;
	uint32 mBindlessAddress = -1;

	VkImageLayout mLayout = VK_IMAGE_LAYOUT_UNDEFINED;

};

struct SBuffer_T : SBase, IDestroyable {

	SBuffer_T() = default;
	EXPORT SBuffer_T(uint32 inBufferSize, VmaMemoryUsage inMemoryUsage, VkBufferUsageFlags inBufferUsage = 0);

	EXPORT void makeGlobal(); //TODO: alternate way of doing this
	EXPORT void updateGlobal() const; // TODO: not global but instead 'dynamic' (address should be separate?)

	EXPORT virtual void destroy() override;

	no_discard EXPORT void* getMappedData() const;

	EXPORT void mapData(void** data) const;

	EXPORT void unMapData() const;

	VkBuffer buffer = nullptr;

	VmaAllocation allocation = nullptr;
	VmaAllocationInfo info = {};

	uint32 mBindlessAddress = 0;
};
/*
// A class that automatically removes itself from the Resource Manager once it's deleted
// Also supports replacement
template <typename TType>
requires std::is_base_of_v<SBase, TType>
struct TRemovable {

	~TRemovable() {
		destroy();
	}

	template <typename... TArgs>
	requires std::is_constructible_v<TType, TArgs...>
	void construct(CResourceManager& manager, TArgs... args) {
		destroy();

		mManager = &manager;
		itr = mManager->create<TType>(mObject, args...);

		mManager->callback([&] {
			destroy();
		});
	}

	TType* get() const { return mObject; }

	bool isValid() const { return mObject != nullptr; }

	void destroy() {
		if (mObject && mManager) {
			mManager->remove(itr);
			mObject = nullptr;
		}
	}

private:
	CResourceManager* mManager = nullptr;
	CResourceManager::Iterator itr;
	TType* mObject = nullptr;
};*/

// Static Buffers are initialized when needed
// Can be done either through templates or constructors for flexibility
template <VmaMemoryUsage TMemoryUsage, VkBufferUsageFlags TBufferUsage, size_t TElementSize = 0, size_t TSize = 0>
struct SStaticBuffer {

	using StagingBufferType = SStaticBuffer<VMA_MEMORY_USAGE_CPU_ONLY, VK_BUFFER_USAGE_TRANSFER_SRC_BIT>;

	SStaticBuffer() = default;

	virtual ~SStaticBuffer() {
		SStaticBuffer::destroy();
	}

	// Since templated Static Buffers are resolved at compile time, the buffer will never change
	// Thus, it should always be allocated globally
	virtual SBuffer_T* get() {
		if (!mAllocated) {
			mAllocated = true;
			itr = CResourceManager::get().create(mBuffer, getSize(), TMemoryUsage, TBufferUsage);
		}
		return mBuffer;
	}

	virtual bool isValid() const {
		return mAllocated;
	}

	virtual size_t getSize() const {
		return TElementSize * TSize;
	}

	virtual void destroy() {
		if (!mAllocated) return;
		mAllocated = false;
		CResourceManager::get().remove(itr);
	}

	template <typename... TArgs>
	void push(const void* src, const size_t size, TArgs... args) {
		size_t totalSize = getTotalSize(src, size, args...);
		if (totalSize > getSize()) {
			errs("Tried to allocate more than available in Static Buffer!");
		}

		if constexpr (TMemoryUsage == VMA_MEMORY_USAGE_GPU_ONLY) {

			static_assert(TBufferUsage & VK_BUFFER_USAGE_TRANSFER_DST_BIT);

			SStaticBuffer<VMA_MEMORY_USAGE_CPU_ONLY, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, TElementSize, TSize> staging;
			staging.push(src, size, args...);

			CRenderer::get()->immediateSubmit([&](SCommandBuffer& cmd) {
				VkBufferCopy copy{};
				copy.dstOffset = 0;
				copy.srcOffset = 0;
				copy.size = totalSize;

				vkCmdCopyBuffer(cmd, staging.get()->buffer, get()->buffer, 1, &copy);
			});

			staging.destroy();
		} else {
			memcpy(get()->getMappedData(), src, size);
			push(size, args...);
		}
	}

private:

	template <typename... TArgs>
	void push(const size_t offset = 0) {}

	template <typename... TArgs>
	void push(const size_t offset, const void* src, const size_t size, TArgs... args) {
		memcpy(static_cast<char*>(get()->getMappedData()) + offset, src, size);
		push(offset + size, args...);
	}

	template <typename... TArgs>
	size_t getTotalSize() {
		return 0;
	}

	template <typename... TArgs>
	size_t getTotalSize(const void* src, const size_t size, TArgs... args) {
		return size + getTotalSize(args...);
	}

	CResourceManager::Iterator itr;
	bool mAllocated = false;
	SBuffer_T* mBuffer = nullptr;
};

typedef SStaticBuffer<VMA_MEMORY_USAGE_CPU_ONLY, VK_BUFFER_USAGE_TRANSFER_SRC_BIT> SStagingBuffer;

template <VmaMemoryUsage TMemoryUsage, VkBufferUsageFlags TBufferUsage>
struct SStaticBuffer<TMemoryUsage, TBufferUsage, 0, 0> {

	SStaticBuffer() = delete; // Static Buffer needs to explicitly know it's size at construction

	SStaticBuffer(CResourceManager& inManager, const size_t inAllocSize): mAllocSize(inAllocSize), mManager(inManager) {}
	SStaticBuffer(CResourceManager& inManager, const size_t inElementSize, const size_t inSize): SStaticBuffer(inManager, inElementSize * inSize) {}

	SStaticBuffer(const size_t inAllocSize): SStaticBuffer(CResourceManager::get(), inAllocSize) {}
	SStaticBuffer(const size_t inElementSize, const size_t inSize): SStaticBuffer(CResourceManager::get(), inElementSize, inSize) {}

	SBuffer_T* get() {
		if (mAllocSize <= 0) {
			errs("Static Buffer has not been given a size!");
		}
		if (!mAllocated) {
			mAllocated = true;
			itr = mManager.create(mBuffer, mAllocSize, TMemoryUsage, TBufferUsage);
		}
		return mBuffer;
	}

	size_t getSize() const {
		return mAllocSize;
	}

	bool isValid() const {
		return mAllocated && mAllocSize > 0;
	}

	void destroy() {
		if (!mAllocated) return;
		mAllocated = false;
		mManager.remove(itr);
	}

	template <typename... TArgs>
	void push(const void* src, const size_t size = 0, TArgs... args) {
		size_t totalSize = getTotalSize(src, size, args...);
		if (totalSize > getSize()) {
			errs("Tried to allocate more than available in Static Buffer!");
		}

		if constexpr (TMemoryUsage == VMA_MEMORY_USAGE_GPU_ONLY) {

			static_assert(TBufferUsage & VK_BUFFER_USAGE_TRANSFER_DST_BIT);

			SStagingBuffer staging{totalSize};
			staging.push(src, size, args...);

			CRenderer::get()->immediateSubmit([&](SCommandBuffer& cmd) {
				VkBufferCopy copy{};
				copy.dstOffset = 0;
				copy.srcOffset = 0;
				copy.size = totalSize;

				vkCmdCopyBuffer(cmd, staging.get()->buffer, get()->buffer, 1, &copy);
			});

			staging.destroy();
		} else {
			memcpy(get()->getMappedData(), src, size);
			push(size, args...);
		}
	}

private:

	template <typename... TArgs>
	void push(const size_t offset = 0) {}

	template <typename... TArgs>
	void push(const size_t offset, const void* src, const size_t size = 0, TArgs... args) {
		memcpy(static_cast<char*>(get()->getMappedData()) + offset, src, size);
		push(offset + size, args...);
	}

	template <typename... TArgs>
	size_t getTotalSize() {
		return 0;
	}

	template <typename... TArgs>
	size_t getTotalSize(const void* src, const size_t size, TArgs... args) {
		return size + getTotalSize(args...);
	}

	const size_t mAllocSize = 0;
	CResourceManager& mManager;
	CResourceManager::Iterator itr;
	bool mAllocated = false;
	SBuffer_T* mBuffer = nullptr;
};

// Dynamic Buffers are initialized when their size changes
template <VmaMemoryUsage TMemoryUsage, VkBufferUsageFlags TBufferUsage = 0>
struct SDynamicBuffer {

	SDynamicBuffer(CResourceManager& inManager): mManager(inManager) {}

	// By default, use global manager
	SDynamicBuffer(): SDynamicBuffer(CResourceManager::get()) {}

	SBuffer_T* get() const {
		if (mAllocSize <= 0 || !mAllocated) {
			errs("Dynamic Buffer has not been allocated!");
		}
		return mBuffer;
	}

	void allocate(const size_t inElementSize, const size_t inSize) {
		return allocate(inElementSize * inSize);
	}

	void allocate(const size_t inAllocSize) {
		if (inAllocSize <= 0) {
			errs("Dynamic Buffer has been given an invalid size!");
		}
		// If allocated and has changed, destroy
		if (mAllocated && mAllocSize != inAllocSize) {
			destroy();
		}
		if (!mAllocated) {
			mAllocated = true;
			mAllocSize = inAllocSize;
			itr = mManager.create(mBuffer, mAllocSize, TMemoryUsage, TBufferUsage);
		}
	}

	size_t getSize() const {
		return mAllocSize;
	}

	bool isValid() const {
		return mAllocated;
	}

	void destroy() {
		if (!mAllocated) return;
		mAllocated = false;
		mManager.remove(itr);
	}

	template <typename... TArgs>
	void push(const void* src, const size_t size = 0, TArgs... args) {
		size_t totalSize = getTotalSize(src, size, args...);
		if (totalSize > getSize()) {
			allocate(totalSize);
		}

		if constexpr (TMemoryUsage == VMA_MEMORY_USAGE_GPU_ONLY) {

			static_assert(TBufferUsage & VK_BUFFER_USAGE_TRANSFER_DST_BIT);

			SStagingBuffer staging{totalSize};
			staging.push(src, size, args...);

			CRenderer::get()->immediateSubmit([&](SCommandBuffer& cmd) {
				VkBufferCopy copy{};
				copy.dstOffset = 0;
				copy.srcOffset = 0;
				copy.size = totalSize;

				vkCmdCopyBuffer(cmd, staging.get()->buffer, get()->buffer, 1, &copy);
			});

			staging.destroy();
		} else {
			memcpy(get()->getMappedData(), src, size);
			push(size, args...);
		}
	}

private:

	template <typename... TArgs>
	void push(const size_t offset, const void* src, const size_t size = 0, TArgs... args) {
		memcpy(static_cast<char*>(get()->getMappedData()) + offset, src, size);
		push(offset + size, args...);
	}

	template <typename... TArgs>
	size_t getTotalSize() {
		return 0;
	}

	template <typename... TArgs>
	size_t getTotalSize(const void* src, const size_t size, TArgs... args) {
		return size + getTotalSize(args...);
	}

	size_t mAllocSize = 0;
	CResourceManager& mManager;
	CResourceManager::Iterator itr;
	bool mAllocated = false;
	SBuffer_T* mBuffer = nullptr;
};

// Holds the resources needed for mesh rendering
struct SMeshBuffers_T : SBase, IDestroyable {

	SMeshBuffers_T() = default;
	EXPORT SMeshBuffers_T(size_t indicesSize, size_t verticesSize);

	SBuffer_T* indexBuffer = nullptr;
	SBuffer_T* vertexBuffer = nullptr;
};

// A command buffer that stores some temporary data to be removed/changed at end of rendering
// Ex: contains info for image transitions
struct SCommandBuffer : SBase {

	SCommandBuffer() = default;

	SCommandBuffer(const CCommandPool* inCmdPool) {
		VkCommandBufferAllocateInfo frameCmdAllocInfo = CVulkanInfo::createCommandAllocateInfo(*inCmdPool, 1);
		VK_CHECK(vkAllocateCommandBuffers(CRenderer::vkDevice(), &frameCmdAllocInfo, &cmd));
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

	VkCommandBuffer cmd = nullptr;
	std::forward_list<SImage_T*> imageTransitions;
};

struct SVulkanAllocator final : SObject, IInitializable, IDestroyable {

	REGISTER_STRUCT(SVulkanAllocator, SObject)

	// Allocator may be called at any point, especially by static buffers, so it needs to be lazy
	MAKE_LAZY_SINGLETON(SVulkanAllocator)

public:

	VmaAllocator& getAllocator() {
		return mAllocator;
	}

	EXPORT virtual void init() override;

	EXPORT virtual void destroy() override;

private:

	VmaAllocator mAllocator = nullptr;
};
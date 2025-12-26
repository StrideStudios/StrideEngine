#pragma once

#include <forward_list>
#include <vma/vk_mem_alloc.h>
#include <vulkan/vk_enum_string_helper.h>
#include <vulkan/vulkan_core.h>

#include "VkBootstrap.h"
#include "rendercore/VulkanDevice.h"
#include "rendercore/VulkanUtils.h"
#include "basic/core/Common.h"
#include "sstl/Deque.h"

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

class CVulkanAllocator final : public SObject, public IDestroyable {

	REGISTER_CLASS(CVulkanAllocator, SObject)

public:

	struct Resource : SObject, IDestroyable {
		Resource(const std::string& name): name(name) {}
		virtual bool isAllocated() = 0;
		//TResourceManager<Resource>::Iterator itr;
		std::string name;
	};

	CVulkanAllocator() = default;
	EXPORT CVulkanAllocator(const TShared<CRenderer>& inRenderer);

	template <typename TType>
	requires std::is_base_of_v<Resource, TType>
	TType* addResource(TUnique<TType>&& inResource) {
		if (mDestroyed) {
			errs("Attempted to allocate a resource after Vulkan Allocator was Destroyed!");
		}
		m_Resources.push(std::move(inResource));
		return m_Resources.bottom().staticCast<TType>();
	}

	template <typename TType>
	requires std::is_base_of_v<Resource, TType>
	void removeResource(TType* inResource) {
		if (mDestroyed) {
			msgs("Attempted to remove a resource after Vulkan Allocator was Destroyed!");
			return;
		}
		m_Resources.transfer(getCurrentResourceManager(), m_Resources.find(inResource));
	}

	VmaAllocator& getAllocator() {
		return mAllocator;
	}

	EXPORT virtual void destroy() override;

	void flushFrameData() {
		getCurrentResourceManager().forEach([](size_t, const TUnique<Resource>& resource) {
			resource->destroy();
		});
		getCurrentResourceManager().clear();
	}

public:

	force_inline TList<TUnique<Resource>>& getCurrentResourceManager() {
		return **m_BufferedManagers[m_Renderer->getBufferingType().getFrameIndex()];
	}

	force_inline const TList<TUnique<Resource>>& getCurrentResourceManager() const {
		return **m_BufferedManagers[m_Renderer->getBufferingType().getFrameIndex()];
	}

	bool mDestroyed = false;

	TWeak<CRenderer> m_Renderer = nullptr;

	VmaAllocator mAllocator = nullptr;

	TList<TUnique<Resource>> m_Resources;
	TDeque<TUnique<TList<TUnique<Resource>>>> m_BufferedManagers;
};

// Create wrappers around Vulkan types that can be destroyed
#define CREATE_VK_TYPE(inName) \
	struct C##inName final : SObject, IDestroyable { \
		REGISTER_STRUCT(C##inName, SObject) \
		C##inName() = default; \
		C##inName(const TShared<CVulkanDevice>& inDevice, Vk##inName inType):  device(inDevice), m##inName(inType) {} \
		C##inName(const TShared<CVulkanDevice>& inDevice, const C##inName& in##inName):  device(inDevice), m##inName(in##inName.m##inName) {} \
		C##inName(const TShared<CVulkanDevice>& inDevice, Vk##inName##CreateInfo inCreateInfo): device(inDevice) { \
			VK_CHECK(vkCreate##inName(device->getDevice().device, &inCreateInfo, nullptr, &m##inName)); \
		} \
		virtual Vk##inName get() { return m##inName; } \
		virtual void destroy() override { vkDestroy##inName(device->getDevice().device, m##inName, nullptr); } \
		Vk##inName operator->() const { return m##inName; } \
		operator Vk##inName() const { return m##inName; } \
		TWeak<CVulkanDevice> device = nullptr; \
		Vk##inName m##inName = nullptr; \
	}

CREATE_VK_TYPE(CommandPool);
CREATE_VK_TYPE(DescriptorPool);
CREATE_VK_TYPE(DescriptorSetLayout);
CREATE_VK_TYPE(Fence);
CREATE_VK_TYPE(PipelineLayout);

struct CPipeline final : SObject, IDestroyable {

	REGISTER_STRUCT(CPipeline, SObject)

	CPipeline() = default;
	EXPORT CPipeline(const TShared<CVulkanDevice>& inDevice, const SPipelineCreateInfo& inCreateInfo, CVertexAttributeArchive& inAttributes, CPipelineLayout* inLayout);

	EXPORT void bind(VkCommandBuffer cmd, const VkPipelineBindPoint inBindPoint) const;

	EXPORT virtual void destroy() override;

	VkPipeline operator->() const { return mPipeline; }

	operator VkPipeline() const { return mPipeline; }

	TWeak<CVulkanDevice> device = nullptr;
	VkPipeline mPipeline = nullptr;

	CPipelineLayout* mLayout;

};

CREATE_VK_TYPE(RenderPass);
CREATE_VK_TYPE(Sampler);
CREATE_VK_TYPE(Semaphore);
CREATE_VK_TYPE(ShaderModule); //TODO: remove

struct CDescriptorSet final : SObject {

	REGISTER_STRUCT(CDescriptorSet, SObject)

	CDescriptorSet() = default;

	CDescriptorSet(const VkDescriptorSet inType): mDescriptorSet(inType) {}

	CDescriptorSet(const CDescriptorSet& inDescriptorSet): mDescriptorSet(inDescriptorSet.mDescriptorSet) {}

	CDescriptorSet(const TShared<CVulkanDevice>& inDevice, const VkDescriptorSetAllocateInfo& inCreateInfo) {
		VK_CHECK(vkAllocateDescriptorSets(inDevice->getDevice().device, &inCreateInfo, &mDescriptorSet));
	}

	void bind(VkCommandBuffer cmd, VkPipelineBindPoint inBindPoint, VkPipelineLayout inPipelineLayout, uint32 inFirstSet, uint32 inDescriptorSetCount) const {
		vkCmdBindDescriptorSets(cmd, inBindPoint,inPipelineLayout, inFirstSet, inDescriptorSetCount, &mDescriptorSet, 0, nullptr);
	}

	VkDescriptorSet operator->() const { return mDescriptorSet; }
	operator VkDescriptorSet() const { return mDescriptorSet; }

	VkDescriptorSet mDescriptorSet = nullptr;
};

struct SShader : SObject, IDestroyable {

	REGISTER_STRUCT(SShader, SObject)

	SShader() = default;
	EXPORT SShader(const TShared<CVulkanDevice>& inDevice, const char* inFilePath);

	EXPORT virtual void destroy() override;

	std::string mFileName = "";
	VkShaderModule mModule = nullptr;
	EShaderStage mStage = EShaderStage::VERTEX;
	std::string mShaderCode = "";
	std::vector<uint32> mCompiledShader{};

private:

	uint32 compile();

	bool loadShader(VkDevice inDevice, const char* inFileName, uint32 Hash);

	bool saveShader(const char* inFileName, uint32 Hash) const;

	TWeak<CVulkanDevice> device = nullptr;
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

struct SImage_T : SObject, IDestroyable {

	REGISTER_STRUCT(SImage_T, SObject)

	SImage_T() = default;
	EXPORT SImage_T(const TShared<CVulkanAllocator>& inAllocator, const TShared<CVulkanDevice>& inDevice, const std::string& inDebugName, VkExtent3D inExtent, VkFormat inFormat, VkImageUsageFlags inFlags = 0, VkImageAspectFlags inViewFlags = 0, uint32 inNumMips = 1);

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

	TWeak<CVulkanAllocator> allocator = nullptr;
	TWeak<CVulkanDevice> device = nullptr;
	VmaAllocation mAllocation = nullptr;
	uint32 mBindlessAddress = -1;

	VkImageLayout mLayout = VK_IMAGE_LAYOUT_UNDEFINED;

};

struct SBuffer_T : CVulkanAllocator::Resource {

	EXPORT SBuffer_T(VmaAllocator inAllocator, const std::string& inName, size_t inBufferSize, VmaMemoryUsage inMemoryUsage, VkBufferUsageFlags inBufferUsage = 0);

	EXPORT void makeGlobal(const TShared<CVulkanDevice>& inDevice); //TODO: alternate way of doing this
	EXPORT void updateGlobal(const TShared<CVulkanDevice>& inDevice) const; // TODO: not global but instead 'dynamic' (address should be separate?)

	EXPORT virtual void destroy() override;

	no_discard EXPORT void* getMappedData() const;

	EXPORT void mapData(void** data) const;

	EXPORT void unMapData() const;

	virtual bool isAllocated() override {
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
//TODO: local buffer and maybe base class

template <VmaMemoryUsage TMemoryUsage, VkBufferUsageFlags TBufferUsage>
struct SPushableBuffer {

	SPushableBuffer(const std::string& inName) : name(inName) {}
	virtual ~SPushableBuffer() = default;

	virtual SBuffer_T* get(const TShared<CVulkanAllocator>& allocator) = 0;

	virtual size_t getSize() const = 0;

	const std::string& getName() const { return name; }

	virtual void checkSize(const TShared<CVulkanAllocator>& allocator, const size_t size) {
		if (size != getSize()) {
			errs("Tried to allocate {} bytes in buffer {} of size {}!", size, name, getSize());
		}
	}

	template <typename... TArgs>
	void push(const TShared<CVulkanAllocator>& allocator, const void* src, const size_t size = 0, TArgs... args) {
		size_t totalSize = getTotalSize(src, size, args...);
		checkSize(allocator, totalSize);

		if constexpr (TMemoryUsage == VMA_MEMORY_USAGE_GPU_ONLY) {

			static_assert(TBufferUsage & VK_BUFFER_USAGE_TRANSFER_DST_BIT);

			SBuffer_T buffer{
				allocator->getAllocator(),
				"Staging for " + name,
				getSize(),
				VMA_MEMORY_USAGE_CPU_ONLY,
				VK_BUFFER_USAGE_TRANSFER_SRC_BIT
			};

			buffer.push(src, size, args...);

			TWeak<CVulkanAllocator> weakAllocator = allocator;
			allocator->m_Renderer->immediateSubmit([this, weakAllocator, totalSize, buffer](SCommandBuffer& cmd) {
				VkBufferCopy copy {
					.srcOffset = 0,
					.dstOffset = 0,
					.size = totalSize
				};

				vkCmdCopyBuffer(cmd, buffer.buffer, get(weakAllocator)->buffer, 1, &copy);
			});

			buffer.destroy();
		} else {
			memcpy(get(allocator)->getMappedData(), src, size);
			push(allocator, size, args...);
		}
	}

private:

	template <typename... TArgs>
	void push(const TShared<CVulkanAllocator>& allocator, const size_t offset = 0) {}

	template <typename... TArgs>
	void push(const TShared<CVulkanAllocator>& allocator, const size_t offset, const void* src, const size_t size, TArgs... args) {
		memcpy(static_cast<char*>(get(allocator)->getMappedData()) + offset, src, size);
		push(allocator, offset + size, args...);
	}

	template <typename... TArgs>
	size_t getTotalSize() {
		return 0;
	}

	template <typename... TArgs>
	size_t getTotalSize(const void* src, const size_t size, TArgs... args) {
		return size + getTotalSize(args...);
	}

	std::string name;
};


// Local Buffers are initialized upon creation
// They do not use the Vulkan Allocator, so are allocated and deallocated based on scope
// This requires that they do not interact with the GPU, or else the GPU will complain about it's destruction when double buffering
// Best used for staging buffers
template <VkBufferUsageFlags TBufferUsage>
struct SLocalBuffer final : SPushableBuffer<VMA_MEMORY_USAGE_CPU_ONLY, TBufferUsage> {

	SLocalBuffer() = delete;

	SLocalBuffer(const TShared<CVulkanAllocator>& allocator, const std::string& inName, const size_t inAllocSize):
	SPushableBuffer<VMA_MEMORY_USAGE_CPU_ONLY, TBufferUsage>(inName),
	mBuffer(allocator->getAllocator(), SPushableBuffer<VMA_MEMORY_USAGE_CPU_ONLY, TBufferUsage>::getName(), inAllocSize, VMA_MEMORY_USAGE_CPU_ONLY, TBufferUsage){}

	SLocalBuffer(const TShared<CVulkanAllocator>& allocator, const std::string& inName, const size_t inElementSize, const size_t inSize): SLocalBuffer(allocator, inName, inElementSize * inSize) {}

	virtual ~SLocalBuffer() override {
		msgs("Destroyed Local Buffer.");
		mBuffer.destroy();
	}

	virtual SBuffer_T* get(const TShared<CVulkanAllocator>&) override { return &mBuffer; }

	virtual size_t getSize() const override { return mBuffer.size; }

private:

	SBuffer_T mBuffer;

};

typedef SLocalBuffer<VK_BUFFER_USAGE_TRANSFER_SRC_BIT> SStagingBuffer;

// Static Buffers are initialized when needed
// They use the Vulkan Allocator, so they persist until the gpu is finished using them
// Best used for non-changing GPU Buffers like scene data
template <VmaMemoryUsage TMemoryUsage, VkBufferUsageFlags TBufferUsage, size_t TElementSize, size_t TSize>
struct SStaticBuffer : SPushableBuffer<TMemoryUsage, TBufferUsage>, IDestroyable {

	SStaticBuffer(const std::string& inName)
	: SPushableBuffer<TMemoryUsage, TBufferUsage>(inName) {}

	virtual ~SStaticBuffer() override {
		if (!mAllocated) return;
		msgs("WARNING: Static Buffer was not destroyed!");
	}

	// Since templated Static Buffers are resolved at compile time, the buffer will never change
	// Thus, it should always be allocated globally
	virtual SBuffer_T* get(const TShared<CVulkanAllocator>& inAllocator) override {
		if (!mAllocated) {
			mAllocated = true;
			allocator = inAllocator;
			mBuffer = allocator->addResource(TUnique<SBuffer_T>{allocator->getAllocator(), SPushableBuffer<TMemoryUsage, TBufferUsage>::getName(), getSize(), TMemoryUsage, TBufferUsage});
		}
		return mBuffer;
	}

	virtual size_t getSize() const override {
		return TElementSize * TSize;
	}

	virtual void destroy() override {
		if (!allocator || !mAllocated) return;
		msgs("Destroyed Static Buffer.");
		mAllocated = false;
		allocator->removeResource(mBuffer);
	}

private:

	TWeak<CVulkanAllocator> allocator = nullptr;

	bool mAllocated = false;
	SBuffer_T* mBuffer = nullptr; //TODO: move buffer stuff locally (MAYBE parent class)
};

// Dynamic Buffers are initialized when their size changes
// They use the Vulkan Allocator, so they persist until the gpu is finished using them
// Best used for changing GPU Buffers like instance data or vertex data
template <VmaMemoryUsage TMemoryUsage, VkBufferUsageFlags TBufferUsage = 0>
struct SDynamicBuffer : SPushableBuffer<TMemoryUsage, TBufferUsage>, IDestroyable {

	SDynamicBuffer(const std::string& inName):
	SPushableBuffer<TMemoryUsage, TBufferUsage>(inName) {}

	virtual ~SDynamicBuffer() override {
		if (!mAllocated) return;
		msgs("WARNING: Dynamic Buffer was not destroyed!");
	}

	virtual SBuffer_T* get(const TShared<CVulkanAllocator>&) override {
		if (mAllocSize <= 0 || !mAllocated) {
			errs("Dynamic Buffer has not been allocated!");
		}
		return mBuffer;
	}

	virtual void checkSize(const TShared<CVulkanAllocator>& inAllocator, const size_t size) override {
		allocate(inAllocator, size);
		SPushableBuffer<TMemoryUsage, TBufferUsage>::checkSize(inAllocator, size);
	}

	virtual size_t getSize() const override {
		return mAllocSize;
	}

	virtual void destroy() override
	{
		if (!allocator || !mAllocated) return;
		msgs("Destroyed Dynamic Buffer.");
		mAllocated = false;
		allocator->removeResource(mBuffer);
	}

private:

	void allocate(const TShared<CVulkanAllocator>& inAllocator, const size_t inAllocSize) {
		if (inAllocSize <= 0) {
			errs("Dynamic Buffer has been given an invalid size!");
		}
		// If allocated and has changed, destroy
		if (mAllocated && mAllocSize != inAllocSize) {
			destroy();
		}
		if (!mAllocated) {
			mAllocated = true;
			allocator = inAllocator;
			mAllocSize = inAllocSize;
			mBuffer = allocator->addResource(TUnique<SBuffer_T>{allocator->getAllocator(), SPushableBuffer<TMemoryUsage, TBufferUsage>::getName(), mAllocSize, TMemoryUsage, TBufferUsage});
		}
	}

	TWeak<CVulkanAllocator> allocator = nullptr;

	size_t mAllocSize = 0;
	bool mAllocated = false;
	SBuffer_T* mBuffer = nullptr;
};

// Holds the resources needed for mesh rendering
struct SMeshBuffers_T : SObject, IDestroyable {

	REGISTER_STRUCT(SMeshBuffers_T, SObject)

	SMeshBuffers_T() = default;
	EXPORT SMeshBuffers_T(const TShared<CVulkanAllocator>& allocator, const std::string& inName, size_t indicesSize, size_t verticesSize);

	std::string name = "None";
	SBuffer_T* indexBuffer = nullptr;
	SBuffer_T* vertexBuffer = nullptr;
};

// A command buffer that stores some temporary data to be removed/changed at end of rendering
// Ex: contains info for image transitions
struct SCommandBuffer : SObject {

	REGISTER_STRUCT(SCommandBuffer, SObject)

	SCommandBuffer() = default;

	SCommandBuffer(const TShared<CVulkanDevice>& inDevice, const CCommandPool* inCmdPool) {
		VkCommandBufferAllocateInfo frameCmdAllocInfo = CVulkanInfo::createCommandAllocateInfo(*inCmdPool, 1);
		VK_CHECK(vkAllocateCommandBuffers(inDevice->getDevice().device, &frameCmdAllocInfo, &cmd));
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
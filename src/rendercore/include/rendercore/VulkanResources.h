#pragma once

#include <forward_list>
#include <vma/vk_mem_alloc.h>
#include <vulkan/vulkan_core.h>

#include "Renderer.h"
#include "VkBootstrap.h"
#include "basic/core/Common.h"
#include "VRI/VRICommands.h"
#include "VRI/VRIResources.h"

template <VmaMemoryUsage TMemoryUsage, VkBufferUsageFlags TBufferUsage>
struct SPushableBuffer {

	SPushableBuffer(const std::string& inName) : name(inName) {}
	virtual ~SPushableBuffer() = default;

	virtual TFrail<SVRIBuffer> get() = 0;

	virtual size_t getSize() const = 0;

	const std::string& getName() const { return name; }

	virtual void checkSize(const size_t size) {
		if (size != getSize()) {
			errs("Tried to allocate {} bytes in buffer {} of size {}!", size, name, getSize());
		}
	}

	template <typename... TArgs>
	void push(const void* src, const size_t size = 0, TArgs... args) {
		size_t totalSize = getTotalSize(src, size, args...);
		checkSize(totalSize);

		if constexpr (TMemoryUsage == VMA_MEMORY_USAGE_GPU_ONLY) {

			static_assert(TBufferUsage & VK_BUFFER_USAGE_TRANSFER_DST_BIT);

			SVRIBuffer buffer{
				getSize(),
				VMA_MEMORY_USAGE_CPU_ONLY,
				VK_BUFFER_USAGE_TRANSFER_SRC_BIT
			};

			buffer.push(src, size, args...);

			CRenderer::get()->immediateSubmit([this, totalSize, &buffer](const TFrail<CVRICommands>& cmd) {
				VkBufferCopy copy {
					.srcOffset = 0,
					.dstOffset = 0,
					.size = totalSize
				};

				cmd->copyBuffer(buffer.buffer, get()->buffer, 1, copy);
			});

			buffer.destroy();
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

	std::string name;
};


// Local Buffers are initialized upon creation
// They do not use the Vulkan Allocator, so are allocated and deallocated based on scope
// This requires that they do not interact with the GPU, or else the GPU will complain about it's destruction when double buffering
// Best used for staging buffers
template <VkBufferUsageFlags TBufferUsage>
struct SLocalBuffer final : SPushableBuffer<VMA_MEMORY_USAGE_CPU_ONLY, TBufferUsage> {

	SLocalBuffer() = delete;

	SLocalBuffer(const std::string& inName, const size_t inAllocSize):
	SPushableBuffer<VMA_MEMORY_USAGE_CPU_ONLY, TBufferUsage>(inName),
	mBuffer(inAllocSize, VMA_MEMORY_USAGE_CPU_ONLY, TBufferUsage){}

	SLocalBuffer(const std::string& inName, const size_t inElementSize, const size_t inSize): SLocalBuffer(inName, inElementSize * inSize) {}

	virtual ~SLocalBuffer() override {
		msgs("Destroyed Local Buffer.");
		mBuffer.destroy();
	}

	virtual TFrail<SVRIBuffer> get() override { return &mBuffer; }

	virtual size_t getSize() const override { return mBuffer.size; }

private:

	SVRIBuffer mBuffer;

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
	virtual TFrail<SVRIBuffer> get() override {
		if (!mAllocated) {
			mAllocated = true;
			mBuffer = TUnique<SVRIBuffer>{getSize(), TMemoryUsage, TBufferUsage};
		}
		return mBuffer;
	}

	virtual size_t getSize() const override {
		return TElementSize * TSize;
	}

	virtual void destroy() override {
		if (!mAllocated) return;
		msgs("Destroyed Static Buffer.");
		mAllocated = false;
		mBuffer->destroy();
	}

private:

	bool mAllocated = false;
	TUnique<SVRIBuffer> mBuffer = nullptr; //TODO: move buffer stuff locally (MAYBE parent class)
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

	virtual TFrail<SVRIBuffer> get() override {
		if (mAllocSize <= 0 || !mAllocated) {
			errs("Dynamic Buffer has not been allocated!");
		}
		return mBuffer;
	}

	virtual void checkSize(const size_t size) override {
		allocate(size);
		SPushableBuffer<TMemoryUsage, TBufferUsage>::checkSize(size);
	}

	virtual size_t getSize() const override {
		return mAllocSize;
	}

	virtual void destroy() override
	{
		if (!mAllocated) return;
		msgs("Destroyed Dynamic Buffer.");
		mAllocated = false;
		mBuffer->destroy();
	}

private:

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
			mBuffer = TUnique<SVRIBuffer>{mAllocSize, TMemoryUsage, TBufferUsage};
		}
	}

	size_t mAllocSize = 0;
	bool mAllocated = false;
	TUnique<SVRIBuffer> mBuffer = nullptr;
};
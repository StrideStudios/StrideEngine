#pragma once

#include <vector>
#include <span>
#include <deque>
#include <vulkan/vulkan_core.h>

#include "Common.h"

// adds bindings for descriptors and constructs a descriptor set layout
// descriptor set layouts are then used by SDescriptorAllocator to allocate descriptors
// they are added to the setallocateinfo to create descriptorsets
// Descriptor sets connect CPU data to the GPU, and are key to sending buffers and images
// SDescriptorWriter is then used to bind buffers and textures, which then get used in updateSet
struct SDescriptorLayoutBuilder {

	std::vector<VkDescriptorSetLayoutBinding> bindings;

	void addBinding(uint32 inBinding, VkDescriptorType inDescriptorType);

	void clear();

	VkDescriptorSetLayout build(VkShaderStageFlags inShaderStages, void* pNext = nullptr, VkDescriptorSetLayoutCreateFlags inFlags = 0);
};

struct SDescriptorAllocator {

	struct PoolSizeRatio{
		VkDescriptorType mType;
		float mRatio;
	};

	VkDescriptorPool mPool;

	void init(uint32 inMaxSets, std::span<PoolSizeRatio> inPoolRatios);

	void clear();

	void destroy();

	VkDescriptorSet allocate(VkDescriptorSetLayout inLayout, void* pNext = nullptr);

private:

	VkDescriptorPool getPool();
	static VkDescriptorPool createPool(uint32 inSetCount, std::span<PoolSizeRatio> inPoolRatios);

	std::vector<PoolSizeRatio> ratios;
	std::vector<VkDescriptorPool> fullPools;
	std::vector<VkDescriptorPool> readyPools;
	uint32 setsPerPool;
};

struct SDescriptorWriter {
	std::deque<VkDescriptorImageInfo> imageInfos;
	std::deque<VkDescriptorBufferInfo> bufferInfos;
	std::vector<VkWriteDescriptorSet> writes;

	void writeImage(uint32 inBinding, VkImageView inImage, VkSampler inSampler, VkImageLayout inLayout, VkDescriptorType inType);
	//TODO: write_sampler?
	/*
	The descriptor types that are allowed for a buffer are these.
	VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER
	VK_DESCRIPTOR_TYPE_STORAGE_BUFFER
	VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC
	VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC
	 */
	void writeBuffer(uint32 inBinding, VkBuffer inBuffer, size_t inSize, size_t inOffset, VkDescriptorType inType);

	void clear();
	void updateSet(VkDescriptorSet inSet);
};
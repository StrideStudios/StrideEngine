#include "DescriptorManager.h"

#include "Engine.h"
#include "VulkanDevice.h"
#include "VulkanUtils.h"

void SDescriptorLayoutBuilder::addBinding(uint32 inBinding, VkDescriptorType inDescriptorType) {
	bindings.push_back({
		.binding = inBinding,
		.descriptorType = inDescriptorType,
		.descriptorCount = 1
	});
}

void SDescriptorLayoutBuilder::clear() {
	bindings.clear();
}

VkDescriptorSetLayout SDescriptorLayoutBuilder::build(VkShaderStageFlags inShaderStages, void *pNext, VkDescriptorSetLayoutCreateFlags inFlags) {
	 for (auto& b : bindings) {
	 	b.stageFlags |= inShaderStages;
	 }

 	VkDescriptorSetLayoutCreateInfo info {
 		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		 .pNext = pNext,
		 .flags = inFlags,
		 .bindingCount = static_cast<uint32>(bindings.size()),
		 .pBindings = bindings.data()
	 };

 	VkDescriptorSetLayout set;
 	VK_CHECK(vkCreateDescriptorSetLayout(CEngine::device(), &info, nullptr, &set));

	return set;
}

void SDescriptorAllocator::init(uint32 inMaxSets, std::span<PoolSizeRatio> inPoolRatios) {
	ratios.clear();

	for (auto r : inPoolRatios) {
		ratios.push_back(r);
	}

	VkDescriptorPool newPool = createPool(inMaxSets, inPoolRatios);

	setsPerPool = static_cast<uint32>(inMaxSets * 1.5);

	readyPools.push_back(newPool);
}

void SDescriptorAllocator::clear() {
	for (const auto pool : readyPools) {
		vkResetDescriptorPool(CEngine::device(), pool, 0);
	}
	for (auto pool : fullPools) {
		vkResetDescriptorPool(CEngine::device(), pool, 0);
		readyPools.push_back(pool);
	}
	fullPools.clear();
}

void SDescriptorAllocator::destroy() {
	for (const auto pool : readyPools) {
		vkDestroyDescriptorPool(CEngine::device(), pool, nullptr);
	}
	readyPools.clear();
	for (const auto pool : fullPools) {
		vkDestroyDescriptorPool(CEngine::device(),pool,nullptr);
	}
	fullPools.clear();
}

VkDescriptorSet SDescriptorAllocator::allocate(VkDescriptorSetLayout inLayout, void* pNext) {
	// Get or create a pool to allocate from
	VkDescriptorPool poolToUse = getPool();

	VkDescriptorSetAllocateInfo allocInfo = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		.pNext = pNext,
		.descriptorPool = poolToUse,
		.descriptorSetCount = 1,
		.pSetLayouts = &inLayout,
	};

	VkDescriptorSet ds;

	// Allocation failed. Try again
	if (const VkResult result = vkAllocateDescriptorSets(CEngine::device(), &allocInfo, &ds); result == VK_ERROR_OUT_OF_POOL_MEMORY || result == VK_ERROR_FRAGMENTED_POOL) {
		fullPools.push_back(poolToUse);

		poolToUse = getPool();
		allocInfo.descriptorPool = poolToUse;

		VK_CHECK(vkAllocateDescriptorSets(CEngine::device(), &allocInfo, &ds));
	}

	readyPools.push_back(poolToUse);
	return ds;
}

VkDescriptorPool SDescriptorAllocator::getPool() {
	VkDescriptorPool newPool;
	if (!readyPools.empty()) {
		newPool = readyPools.back();
		readyPools.pop_back();
	} else {
		// Create new pool
		newPool = createPool(setsPerPool, ratios);

		setsPerPool = uint32(setsPerPool * 1.5);

		if (setsPerPool > 4092) {
			setsPerPool = 4092;
		}
	}
	return newPool;
}

VkDescriptorPool SDescriptorAllocator::createPool(const uint32 inSetCount, const std::span<PoolSizeRatio> inPoolRatios) {
	std::vector<VkDescriptorPoolSize> poolSizes;
	for (auto [mType, mRatio] : inPoolRatios) {
		poolSizes.push_back(VkDescriptorPoolSize{
			.type = mType,
			.descriptorCount = static_cast<uint32>(mRatio * inSetCount)
		});
	}

	VkDescriptorPoolCreateInfo poolInfo {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.maxSets = inSetCount,
		.poolSizeCount = (uint32)poolSizes.size(),
		.pPoolSizes = poolSizes.data()
	};

	VkDescriptorPool newPool;
	vkCreateDescriptorPool(CEngine::device(), &poolInfo, nullptr, &newPool);
	return newPool;
}

//TODO: info in SImage then just pass in here
void SDescriptorWriter::writeImage(uint32 inBinding, VkImageView inImage, VkSampler inSampler, VkImageLayout inLayout, VkDescriptorType inType) {
	VkDescriptorImageInfo& info = imageInfos.emplace_back(VkDescriptorImageInfo{
		.sampler = inSampler,
		.imageView = inImage,
		.imageLayout = inLayout
	});

	writes.push_back({
		.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		.dstSet = VK_NULL_HANDLE, //left empty for now until we need to write it
		.dstBinding = inBinding,
		.descriptorCount = 1,
		.descriptorType = inType,
		.pImageInfo = &info
	});
}

//TODO: pass in SBuffer here
void SDescriptorWriter::writeBuffer(uint32 inBinding, VkBuffer inBuffer, size_t inSize, size_t inOffset, VkDescriptorType inType) {
	VkDescriptorBufferInfo& info = bufferInfos.emplace_back(VkDescriptorBufferInfo{
		.buffer = inBuffer,
		.offset = inOffset,
		.range = inSize
	});

	writes.push_back({
		.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		.dstSet = VK_NULL_HANDLE, //left empty for now until we need to write it
		.dstBinding = inBinding,
		.descriptorCount = 1,
		.descriptorType = inType,
		.pBufferInfo = &info
	});
}

void SDescriptorWriter::clear() {
	imageInfos.clear();
	writes.clear();
	bufferInfos.clear();
}

void SDescriptorWriter::updateSet(VkDescriptorSet inSet) {
	for (VkWriteDescriptorSet& write : writes) {
		write.dstSet = inSet;
	}

	vkUpdateDescriptorSets(CEngine::device(), (uint32)writes.size(), writes.data(), 0, nullptr);
}
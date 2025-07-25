#include "ResourceManager.h"

#define VMA_IMPLEMENTATION
#include <vma/vk_mem_alloc.h>

#include <vulkan/vk_enum_string_helper.h>

#include "Engine.h"
#include "GraphicsRenderer.h"
#include "VulkanDevice.h"
#include "VulkanRenderer.h"
#include "VulkanUtils.h"

void* SBuffer_T::GetMappedData() const {
	return allocation->GetMappedData();
}

#define CREATE_SWITCH(inType, inName, inEnum) \
	case Type::inEnum: \
		deallocate##inName(static_cast<inType>(ptr)); \
		break;

void CResourceManager::Resource::destroy() const {
	switch (mType) {
		FOR_EACH_TYPE(CREATE_SWITCH)
		CREATE_SWITCH(VkPipeline, Pipeline, PIPELINE)
		case Type::BUFFER:
			deallocateBuffer(static_cast<const SBuffer_T *>(mResource.get()));
			break;
		case Type::IMAGE:
			deallocateImage(static_cast<const SImage_T *>(mResource.get()));
			break;
		default:
			astsNoEntry();
	}
}

#undef CREATE_SWITCH

// TODO: turn this into a local that can be used at any point, forcing its own deallocation

void CResourceManager::initAllocator() {
	// initialize the memory allocator
	VmaAllocatorCreateInfo allocatorInfo {
		.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT,
		.physicalDevice = CEngine::physicalDevice(),
		.device = CEngine::device(),
		.instance = CEngine::instance()
	};

	VK_CHECK(vmaCreateAllocator(&allocatorInfo, &getAllocator()));
}

void CResourceManager::destroyAllocator() {
	vmaDestroyAllocator(getAllocator());
}

#define DEFINE_ALLOCATER(inType, inName, inEnum) \
	inType CResourceManager::allocate##inName(const inType##CreateInfo& pCreateInfo, const VkAllocationCallbacks* pAllocator) { \
		inType inName; \
		VK_CHECK(vkCreate##inName(CEngine::device(), &pCreateInfo, pAllocator, &inName)); \
		push(inName); \
		return inName; \
	}
#define DEFINE_DEALLOCATOR(inType, inName, inEnum) \
	void CResourceManager::deallocate##inName(const inType& inName) { \
		vkDestroy##inName(CEngine::device(), inName, nullptr); \
	}

	FOR_EACH_TYPE(DEFINE_ALLOCATER)
	FOR_EACH_TYPE(DEFINE_DEALLOCATOR)

	// Pipeline has a non-standard allocater
	DEFINE_DEALLOCATOR(VkPipeline, Pipeline, PIPELINE);

#undef DEFINE_FUNCTIONS
#undef DEFINE_DEALLOCATOR

VkCommandBuffer CResourceManager::allocateCommandBuffer(const VkCommandBufferAllocateInfo& pCreateInfo) {
	VkCommandBuffer Buffer;
	VK_CHECK(vkAllocateCommandBuffers(CEngine::device(), &pCreateInfo, &Buffer));
	return Buffer;
}

VkPipeline CResourceManager::allocatePipeline(const CPipelineBuilder& inPipelineBuilder, const VkAllocationCallbacks* pAllocator) {
	const VkPipeline Pipeline = inPipelineBuilder.buildPipeline(CEngine::device());
	push(Pipeline);
	return Pipeline;
}

SImage CResourceManager::allocateImage(VkExtent3D inExtent, VkFormat inFormat, VkImageUsageFlags inFlags, VkImageAspectFlags inViewFlags, bool inMipmapped) {
	auto image = std::make_shared<SImage_T>();

	image->mImageExtent = inExtent;
	image->mImageFormat = inFormat;

	VkImageCreateInfo imageInfo = CVulkanInfo::createImageInfo(image->mImageFormat, inFlags, image->mImageExtent);
	if (inMipmapped) {
		imageInfo.mipLevels = static_cast<uint32>(std::floor(std::log2(std::max(inExtent.width, inExtent.height)))) + 1;
	}

	//for the draw image, we want to allocate it from gpu local memory
	VmaAllocationCreateInfo imageAllocationInfo = {
		.usage = VMA_MEMORY_USAGE_GPU_ONLY,
		.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT),
	};

	vmaCreateImage(getAllocator(), &imageInfo, &imageAllocationInfo, &image->mImage, &image->mAllocation, nullptr);

	//build a image-view for the draw image to use for rendering
	VkImageViewCreateInfo imageViewInfo = CVulkanInfo::createImageViewInfo(image->mImageFormat, image->mImage, inViewFlags);
	imageViewInfo.subresourceRange.levelCount = imageInfo.mipLevels;

	VK_CHECK(vkCreateImageView(CEngine::device(), &imageViewInfo, nullptr, &image->mImageView));

	push(image);

	return image;
}

SImage CResourceManager::allocateImage(void* inData, VkExtent3D inExtent, VkFormat inFormat, VkImageUsageFlags inFlags, VkImageAspectFlags inViewFlags, bool inMipmapped) {
	size_t data_size = inExtent.depth * inExtent.width * inExtent.height * 4;

	// Upload buffer is not needed outside of this function
	CResourceManager manager;
	const SBuffer uploadBuffer = manager.allocateBuffer(data_size, VMA_MEMORY_USAGE_CPU_TO_GPU, VK_BUFFER_USAGE_TRANSFER_SRC_BIT);

	memcpy(uploadBuffer->info.pMappedData, inData, data_size);

	SImage new_image = allocateImage(inExtent, inFormat, inFlags | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, inViewFlags, inMipmapped);

	CVulkanRenderer::immediateSubmit([&](VkCommandBuffer cmd) {
		CVulkanUtils::transitionImage(cmd, new_image->mImage, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

		VkBufferImageCopy copyRegion = {};
		copyRegion.bufferOffset = 0;
		copyRegion.bufferRowLength = 0;
		copyRegion.bufferImageHeight = 0;

		copyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		copyRegion.imageSubresource.mipLevel = 0;
		copyRegion.imageSubresource.baseArrayLayer = 0;
		copyRegion.imageSubresource.layerCount = 1;
		copyRegion.imageExtent = inExtent;

		// copy the buffer into the image
		vkCmdCopyBufferToImage(cmd, uploadBuffer->buffer, new_image->mImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1,
			&copyRegion);

		CVulkanUtils::transitionImage(cmd, new_image->mImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		});

	manager.flush();

	return new_image;
}

void CResourceManager::deallocateImage(const SImage_T* inImage) {
	vmaDestroyImage(getAllocator(), inImage->mImage, inImage->mAllocation);
	vkDestroyImageView(CEngine::device(), inImage->mImageView, nullptr);
}

SBuffer CResourceManager::allocateBuffer(size_t allocSize, VmaMemoryUsage memoryUsage, VkBufferUsageFlags usage) {
	// allocate buffer
	VkBufferCreateInfo bufferInfo = {
		.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		.pNext = nullptr,
		.size = allocSize,
		.usage = usage
	};

	VmaAllocationCreateInfo vmaallocInfo = {
		.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT,
		.usage = memoryUsage
	};

	auto buffer = std::make_shared<SBuffer_T>();

	// allocate the buffer
	VK_CHECK(vmaCreateBuffer(getAllocator(), &bufferInfo, &vmaallocInfo, &buffer->buffer, &buffer->allocation, &buffer->info));

	push(buffer);

	return buffer;
}

SMeshBuffers CResourceManager::allocateMeshBuffer(size_t indicesSize, size_t verticesSize) {
	const VkDevice device = CEngine::device();

	// Although it would normally be bad practice to return a heap allocated pointer
	// CAllocator (so long as inShouldDeallocate is true) will automatically destroy the pointer
	// if not, then the user can call CAllocator::deallocate
	auto meshBuffers = std::make_shared<SMeshBuffers_T>();

	meshBuffers->indexBuffer = allocateBuffer(indicesSize, VMA_MEMORY_USAGE_GPU_ONLY, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);

	meshBuffers->vertexBuffer = allocateBuffer(verticesSize, VMA_MEMORY_USAGE_GPU_ONLY, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT);

	//find the address of the vertex buffer
	// TODO: pointer math for large buffers
	VkBufferDeviceAddressInfo deviceAddressInfo {
		.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
		.buffer = meshBuffers->vertexBuffer->buffer
	};

	meshBuffers->vertexBufferAddress = vkGetBufferDeviceAddress(device, &deviceAddressInfo);

	return meshBuffers;
}

void CResourceManager::deallocateBuffer(const SBuffer_T* inBuffer) {
	vmaDestroyBuffer(getAllocator(), inBuffer->buffer, inBuffer->allocation);
}

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
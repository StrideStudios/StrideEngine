#include "EngineTextures.h"

#include "Engine.h"
#include "VulkanDevice.h"

#define VMA_IMPLEMENTATION
#include <array>

#include "vma/vk_mem_alloc.h"

CEngineTextures::CEngineTextures(CVulkanRenderer* renderer) {

	// initialize the memory allocator
	{
		VmaAllocatorCreateInfo allocatorInfo {
			.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT,
			.physicalDevice = CEngine::get().getDevice().getPhysicalDevice(),
			.device = CEngine::get().getDevice().getDevice(),
			.instance = CEngine::get().getDevice().getInstance()
		};

		vmaCreateAllocator(&allocatorInfo, &mAllocator);
	}

	initializeTextures();

	VkDevice device = CEngine::get().getDevice().getDevice();
	VkCommandPoolCreateInfo uploadCommandPoolInfo = CVulkanInfo::createCommandPoolInfo(renderer->m_GraphicsQueueFamily);
	//create pool for upload context
	VK_CHECK(vkCreateCommandPool(device, &uploadCommandPoolInfo, nullptr, &m_UploadContext._commandPool));

	m_ResourceDeallocator.push(&m_UploadContext._commandPool);

	//allocate the default command buffer that we will use for the instant commands
	VkCommandBufferAllocateInfo cmdAllocInfo = CVulkanInfo::createCommandAllocateInfo(m_UploadContext._commandPool, 1);
	VK_CHECK(vkAllocateCommandBuffers(device, &cmdAllocInfo, &m_UploadContext._commandBuffer));

		VkFenceCreateInfo fenceCreateInfo = CVulkanInfo::createFenceInfo(VK_FENCE_CREATE_SIGNALED_BIT);
	VK_CHECK(vkCreateFence(device, &fenceCreateInfo, nullptr, &m_UploadContext._uploadFence));

	initDefaultData(renderer);
}

void CEngineTextures::initDefaultData(CVulkanRenderer* renderer) {
	std::array<SVertex,4> rect_vertices;

	rect_vertices[0].position = {0.5,-0.5, 0};
	rect_vertices[1].position = {0.5,0.5, 0};
	rect_vertices[2].position = {-0.5,-0.5, 0};
	rect_vertices[3].position = {-0.5,0.5, 0};

	rect_vertices[0].color = {0,0, 0,1};
	rect_vertices[1].color = { 0.5,0.5,0.5 ,1};
	rect_vertices[2].color = { 1,0, 0,1 };
	rect_vertices[3].color = { 0,1, 0,1 };

	std::array<uint32_t,6> rect_indices;

	rect_indices[0] = 0;
	rect_indices[1] = 1;
	rect_indices[2] = 2;

	rect_indices[3] = 2;
	rect_indices[4] = 1;
	rect_indices[5] = 3;

	mRectangle = uploadMesh(renderer, rect_indices, rect_vertices);

	// Delete the rectangle data on engine shutdown
	m_BufferResourceDeallocator.push([&]{
		destroyBuffer(mRectangle.indexBuffer);
		destroyBuffer(mRectangle.vertexBuffer);
	});

}

void CEngineTextures::initializeTextures() {

	// Ensure previous textures have been destroyed
	m_ResourceDeallocator.flush();

	//hardcoding the draw format to 32 bit float
	mDrawImage.mImageFormat = VK_FORMAT_R16G16B16A16_SFLOAT;

	VkImageUsageFlags drawImageUsages = 0;
	drawImageUsages |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
	drawImageUsages |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	drawImageUsages |= VK_IMAGE_USAGE_STORAGE_BIT;
	drawImageUsages |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	auto [width, height] = CEngine::get().getWindow().mExtent;

	mDrawImage.mImageExtent = {width, height, 1};
	VkImageCreateInfo imageInfo = CVulkanInfo::createImageInfo(mDrawImage.mImageFormat, drawImageUsages, VkExtent3D(width, height, 1));

	//for the draw image, we want to allocate it from gpu local memory
	VmaAllocationCreateInfo imageAllocationInfo = {
		.usage = VMA_MEMORY_USAGE_GPU_ONLY,
		.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT),
	};

	//allocate and create the image
	vmaCreateImage(mAllocator, &imageInfo, &imageAllocationInfo, &mDrawImage.mImage, &mDrawImage.mAllocation, nullptr);

	//build a image-view for the draw image to use for rendering
	VkImageViewCreateInfo imageViewInfo = CVulkanInfo::createImageViewInfo(mDrawImage.mImageFormat, mDrawImage.mImage, VK_IMAGE_ASPECT_COLOR_BIT);

	VK_CHECK(vkCreateImageView(CEngine::get().getDevice().getDevice(), &imageViewInfo, nullptr, &mDrawImage.mImageView));

	//add to resource deallocator
	m_ResourceDeallocator.append({
		&mDrawImage.mImageView,
		{mAllocator, &mDrawImage.mImage, mDrawImage.mAllocation}
	});
}

void CEngineTextures::immediateSubmit(CVulkanRenderer* renderer, std::function<void(VkCommandBuffer cmd)>&& function) {
	VkDevice device = CEngine::get().getDevice().getDevice();

	VkCommandBuffer cmd = m_UploadContext._commandBuffer;

	//begin the command buffer recording. We will use this command buffer exactly once before resetting, so we tell vulkan that
	VkCommandBufferBeginInfo cmdBeginInfo = CVulkanInfo::createCommandBufferBeginInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

	VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));

	//execute the function
	function(cmd);

	VK_CHECK(vkEndCommandBuffer(cmd));

	VkSubmitInfo submit = CVulkanInfo::submitInfo(&cmd);

	//submit command buffer to the queue and execute it.
	// _uploadFence will now block until the graphic commands finish execution
	VK_CHECK(vkQueueSubmit(renderer->m_GraphicsQueue, 1, &submit, m_UploadContext._uploadFence));

	vkWaitForFences(device, 1, &m_UploadContext._uploadFence, true, 9999999999);
	vkResetFences(device, 1, &m_UploadContext._uploadFence);

	// reset the command buffers inside the command pool
	vkResetCommandPool(device, m_UploadContext._commandPool, 0);
}

SAllocatedBuffer CEngineTextures::createBuffer(size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage) {
	// allocate buffer
	VkBufferCreateInfo bufferInfo = {.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
	bufferInfo.pNext = nullptr;
	bufferInfo.size = allocSize;

	bufferInfo.usage = usage;

	VmaAllocationCreateInfo vmaallocInfo = {};
	vmaallocInfo.usage = memoryUsage;
	vmaallocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;

	VkBuffer buffer;
	VmaAllocation allocation;
	VmaAllocationInfo info;

	// allocate the buffer
	VK_CHECK(vmaCreateBuffer(mAllocator, &bufferInfo, &vmaallocInfo, &buffer, &allocation, &info));

	return {
		.buffer = buffer,
		.allocation = allocation,
		.info = info
	};
}

SGPUMeshBuffers CEngineTextures::uploadMesh(CVulkanRenderer* renderer, std::span<uint32> indices, std::span<SVertex> vertices) {
	VkDevice device = CEngine::get().getDevice().getDevice();

	const size_t vertexBufferSize = vertices.size() * sizeof(SVertex);
	const size_t indexBufferSize = indices.size() * sizeof(uint32);

	//create index buffer
	SAllocatedBuffer indexBuffer = createBuffer(indexBufferSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VMA_MEMORY_USAGE_GPU_ONLY);

	//create vertex buffer
	SAllocatedBuffer vertexBuffer = createBuffer(vertexBufferSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, VMA_MEMORY_USAGE_GPU_ONLY);

	//find the address of the vertex buffer
	// TODO: pointer math for large buffers
	VkBufferDeviceAddressInfo deviceAdressInfo {
		.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
		.buffer = vertexBuffer.buffer
	};

	// Create buffers
	SGPUMeshBuffers meshBuffers {
		.indexBuffer = indexBuffer,
		.vertexBuffer = vertexBuffer,
		.vertexBufferAddress = vkGetBufferDeviceAddress(device, &deviceAdressInfo)
	};

	SAllocatedBuffer staging = createBuffer(vertexBufferSize + indexBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY);

	void* data = staging.allocation->GetMappedData();

	// copy vertex buffer
	memcpy(data, vertices.data(), vertexBufferSize);
	// copy index buffer
	memcpy((char*)data + vertexBufferSize, indices.data(), indexBufferSize);

	//TODO: slow, render thread?
	// also from an older version of the tutorial, doesnt use sync2
	immediateSubmit(renderer, [&](VkCommandBuffer cmd) {
		VkBufferCopy vertexCopy{ 0 };
		vertexCopy.dstOffset = 0;
		vertexCopy.srcOffset = 0;
		vertexCopy.size = vertexBufferSize;

		vkCmdCopyBuffer(cmd, staging.buffer, meshBuffers.vertexBuffer.buffer, 1, &vertexCopy);

		VkBufferCopy indexCopy{};
		indexCopy.dstOffset = 0;
		indexCopy.srcOffset = vertexBufferSize;
		indexCopy.size = indexBufferSize;

		vkCmdCopyBuffer(cmd, staging.buffer, meshBuffers.indexBuffer.buffer, 1, &indexCopy);
	});

	destroyBuffer(staging);

	return meshBuffers;
}

void CEngineTextures::destroyBuffer(const SAllocatedBuffer &inBuffer) {
	vmaDestroyBuffer(mAllocator, inBuffer.buffer, inBuffer.allocation);
}

void CEngineTextures::reallocate() {

	auto [x, y] = CEngine::get().getWindow().mExtent;
	msg("Reallocating Engine Textures to ({}, {})", x, y);

	m_Swapchain.recreate();

	initializeTextures();
}

void CEngineTextures::destroy() {

	m_ResourceDeallocator.flush();

	m_BufferResourceDeallocator.flush();

	vmaDestroyAllocator(mAllocator);

	m_Swapchain.cleanup();
}


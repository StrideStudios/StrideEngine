#include "ResourceManager.h"

#define VMA_IMPLEMENTATION
#include <vma/vk_mem_alloc.h>

#define STB_IMAGE_IMPLEMENTATION
#include <filesystem>
#include <stb_image.h>

#include <vulkan/vk_enum_string_helper.h>

#include "encoder/3rdparty/tinydds.h"

#include "encoder/basisu_comp.h"
#include "transcoder/basisu_transcoder.h"
#include "encoder/basisu_gpu_texture.h"

#include "Engine.h"
#include "EngineBuffers.h"
#include "Hashing.h"
#include "Paths.h"
#include "PipelineBuilder.h"
#include "VulkanDevice.h"
#include "VulkanRenderer.h"
#include "VulkanUtils.h"
#include "tracy/Tracy.hpp"

void* SBuffer_T::GetMappedData() const {
	return allocation->GetMappedData();
}

void SBuffer_T::mapData(void** data) const {
	vmaMapMemory(CResourceManager::getAllocator(), allocation, data);
}

void SBuffer_T::unMapData() const {
	vmaUnmapMemory(CResourceManager::getAllocator(), allocation);
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

void CResourceManager::init() {

	// initialize the memory allocator
	{
		VmaAllocatorCreateInfo allocatorInfo {
			.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT,
			.physicalDevice = CEngine::physicalDevice(),
			.device = CEngine::device(),
			.instance = CEngine::instance()
		};

		VK_CHECK(vmaCreateAllocator(&allocatorInfo, &getAllocator()));
	}

	// Create Descriptor pool
	{
		auto poolSizes = {
			VkDescriptorPoolSize{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, gMaxBindlessResources},
			VkDescriptorPoolSize{ VK_DESCRIPTOR_TYPE_SAMPLER, gMaxSamplers}
		};

		VkDescriptorPoolCreateInfo poolCreateInfo {
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
			.flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT_EXT,
			.maxSets = gMaxBindlessResources + gMaxSamplers,
			.poolSizeCount = (uint32)poolSizes.size(),
			.pPoolSizes = poolSizes.begin()
		};

		VK_CHECK(vkCreateDescriptorPool(CEngine::device(), &poolCreateInfo, nullptr, &getBindlessDescriptorPool()));
	}

	// Create Descriptor Set layout
	{
		constexpr VkDescriptorBindingFlags flags = VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT; //VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT

		const auto inputFlags = {flags, flags};

        auto binding = {
        	VkDescriptorSetLayoutBinding {
        		.binding = gTextureBinding,
				.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
				.descriptorCount = gMaxBindlessResources,
				.stageFlags = VK_SHADER_STAGE_ALL,
        	},
			VkDescriptorSetLayoutBinding {
				.binding = gSamplerBinding,
				.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER,
				.descriptorCount = gMaxSamplers,
				.stageFlags = VK_SHADER_STAGE_ALL,
			}
        };

		const auto flagInfo = VkDescriptorSetLayoutBindingFlagsCreateInfo {
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO,
			.bindingCount = (uint32)inputFlags.size(),
			.pBindingFlags = inputFlags.begin(),
		};

        VkDescriptorSetLayoutCreateInfo layoutCreateInfo {
        	.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        	.pNext = &flagInfo,
        	.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT_EXT,
        	.bindingCount = (uint32)binding.size(),
        	.pBindings = binding.begin(),
        };

        VK_CHECK(vkCreateDescriptorSetLayout(CEngine::device(), &layoutCreateInfo, nullptr, &getBindlessDescriptorSetLayout()));
	}

	{
		constexpr uint32 maxBinding = gMaxBindlessResources - 1;
		VkDescriptorSetVariableDescriptorCountAllocateInfoEXT countInfo {
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO_EXT,
			.descriptorSetCount = 1,
			.pDescriptorCounts = &maxBinding
		};

		VkDescriptorSetAllocateInfo AllocationCreateInfo {
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
			.pNext = &countInfo,
			.descriptorPool = getBindlessDescriptorPool(),
			.descriptorSetCount = 1,
			.pSetLayouts = &getBindlessDescriptorSetLayout()
		};

		VK_CHECK( vkAllocateDescriptorSets(CEngine::device(), &AllocationCreateInfo, &getBindlessDescriptorSet()));
	}

	// TODO: init samplers, done in engine textures for now
}

void CResourceManager::destroyAllocator() {
	vmaDestroyAllocator(getAllocator());
	vkDestroyDescriptorPool(CEngine::device(), getBindlessDescriptorPool(), nullptr);
	vkDestroyDescriptorSetLayout(CEngine::device(), getBindlessDescriptorSetLayout(), nullptr);
}

#define DEBUG_SHOW_ALLOCATIONS 1

#if DEBUG_SHOW_ALLOCATIONS
#define ZoneScopedAllocation(inScope) \
	ZoneScoped; \
	ZoneName(inScope.c_str(), inScope.size())
#else
#define ZoneScopedAllocation(inScope)
#endif

#define DEFINE_ALLOCATER(inType, inName, inEnum) \
	inType CResourceManager::allocate##inName(const inType##CreateInfo& pCreateInfo, const VkAllocationCallbacks* pAllocator) { \
		ZoneScopedAllocation(std::string("Allocate " #inName)); \
		inType inName; \
		VK_CHECK(vkCreate##inName(CEngine::device(), &pCreateInfo, pAllocator, &inName)); \
		push(inName); \
		return inName; \
	}
#define DEFINE_DEALLOCATOR(inType, inName, inEnum) \
	void CResourceManager::deallocate##inName(const inType& inName) { \
		ZoneScopedAllocation(std::string("Deallocate " #inName)); \
		vkDestroy##inName(CEngine::device(), inName, nullptr); \
	}

	FOR_EACH_TYPE(DEFINE_ALLOCATER)
	FOR_EACH_TYPE(DEFINE_DEALLOCATOR)

	// Pipeline has a non-standard allocater
	DEFINE_DEALLOCATOR(VkPipeline, Pipeline, PIPELINE);

#undef DEFINE_FUNCTIONS
#undef DEFINE_DEALLOCATOR

VkCommandBuffer CResourceManager::allocateCommandBuffer(const VkCommandBufferAllocateInfo& pCreateInfo) {
	ZoneScopedAllocation(std::string("Allocate CommandBuffer"));
	VkCommandBuffer Buffer;
	VK_CHECK(vkAllocateCommandBuffers(CEngine::device(), &pCreateInfo, &Buffer));
	return Buffer;
}

void CResourceManager::bindDescriptorSets(VkCommandBuffer cmd, VkPipelineBindPoint inBindPoint, VkPipelineLayout inPipelineLayout, uint32 inFirstSet, uint32 inDescriptorSetCount, const VkDescriptorSet& inDescriptorSets) {
	ZoneScopedAllocation(std::string("Bind Descriptor Sets"));
	vkCmdBindDescriptorSets(cmd, inBindPoint,inPipelineLayout, inFirstSet, inDescriptorSetCount, &inDescriptorSets, 0, nullptr);
}

VkPipeline CResourceManager::allocatePipeline(const CPipelineBuilder& inPipelineBuilder, const VkAllocationCallbacks* pAllocator) {
	ZoneScopedAllocation(std::string("Allocate Pipeline"));
	const VkPipeline Pipeline = inPipelineBuilder.buildPipeline(CEngine::device());
	push(Pipeline);
	return Pipeline;
}

void CResourceManager::bindPipeline(VkCommandBuffer cmd, const VkPipelineBindPoint inBindPoint, const VkPipeline inPipeline) {
	ZoneScopedAllocation(std::string("Bind Pipeline"));
	vkCmdBindPipeline(cmd, inBindPoint, inPipeline);
}

SBuffer CResourceManager::allocateBuffer(size_t allocSize, VmaMemoryUsage memoryUsage, VkBufferUsageFlags usage) {
	ZoneScopedAllocation(std::string("Allocate Buffer"));
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

SMeshBuffers CResourceManager::allocateMeshBuffer(const size_t indicesSize, const size_t verticesSize) {

	auto meshBuffers = std::make_shared<SMeshBuffers_T>();

	meshBuffers->indexBuffer = allocateBuffer(indicesSize, VMA_MEMORY_USAGE_GPU_ONLY, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);

	meshBuffers->vertexBuffer = allocateBuffer(verticesSize, VMA_MEMORY_USAGE_GPU_ONLY, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);

	//TODO: for now instance buffer only needs two
	meshBuffers->instanceBuffer = allocateBuffer(2 * sizeof(SInstance), VMA_MEMORY_USAGE_CPU_TO_GPU, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);

	return meshBuffers;
}

void CResourceManager::deallocateBuffer(const SBuffer_T* inBuffer) {
	ZoneScopedAllocation(std::string("Deallocate Buffer"));
	vmaDestroyBuffer(getAllocator(), inBuffer->buffer, inBuffer->allocation);
}

SImage CResourceManager::allocateImage(const std::string& inDebugName, VkExtent3D inExtent, VkFormat inFormat, VkImageUsageFlags inFlags, VkImageAspectFlags inViewFlags, bool inMipmapped) {
	if (inMipmapped) {
		return allocateImage(inDebugName, inExtent, inFormat, inFlags, inViewFlags, static_cast<uint32>(std::floor(std::log2(std::max(inExtent.width, inExtent.height)))) + 1);
	}
	return allocateImage(inDebugName, inExtent, inFormat, inFlags, inViewFlags, static_cast<uint32>(1));
}

SImage CResourceManager::allocateImage(const std::string& inDebugName, VkExtent3D inExtent, VkFormat inFormat, VkImageUsageFlags inFlags, VkImageAspectFlags inViewFlags, uint32 inNumMips) {
	ZoneScopedAllocation(fmts("Allocate Image {}", inDebugName.c_str()));
	auto image = std::make_shared<SImage_T>();

	image->name = inDebugName;
	image->mImageExtent = inExtent;
	image->mImageFormat = inFormat;

	VkImageCreateInfo imageInfo = CVulkanInfo::createImageInfo(image->mImageFormat, inFlags, image->mImageExtent);
	imageInfo.mipLevels = inNumMips;

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

	// Update descriptors with new image
	if ((inFlags & VK_IMAGE_USAGE_SAMPLED_BIT) != 0) { //TODO: VK_IMAGE_USAGE_SAMPLED_BIT is not a permanent solution
		// Set and increment current texture address
		static uint32 gCurrentTextureAddress = 0;
		image->mBindlessAddress = gCurrentTextureAddress;
		gCurrentTextureAddress++;

		const auto imageDescriptorInfo = VkDescriptorImageInfo{
			.imageView = image->mImageView,
			.imageLayout = VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL
		};

		const auto writeSet = VkWriteDescriptorSet{
			.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.dstSet = getBindlessDescriptorSet(),
			.dstBinding = gTextureBinding,
			.dstArrayElement = image->mBindlessAddress,
			.descriptorCount = 1,
			.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
			.pImageInfo = &imageDescriptorInfo,
		};
		vkUpdateDescriptorSets(CEngine::device(), 1, &writeSet, 0, nullptr);
	}

	m_Images.push_back(image);

	return image;
}

void generateMipmaps(VkCommandBuffer cmd, VkImage image, VkExtent2D extent) {
	ZoneScopedAllocation(std::string("Generate Mipmaps"));
	int mipLevels = int(std::floor(std::log2(std::max(extent.width, extent.height)))) + 1;
    for (int mip = 0; mip < mipLevels; mip++) {

        VkExtent2D halfSize = extent;
        halfSize.width /= 2;
        halfSize.height /= 2;

        VkImageMemoryBarrier2 imageBarrier { .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2, .pNext = nullptr };

        imageBarrier.srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
        imageBarrier.srcAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT;
        imageBarrier.dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
        imageBarrier.dstAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT | VK_ACCESS_2_MEMORY_READ_BIT;

        imageBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        imageBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;

        VkImageAspectFlags aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        imageBarrier.subresourceRange = CVulkanUtils::imageSubresourceRange(aspectMask);
        imageBarrier.subresourceRange.levelCount = 1;
        imageBarrier.subresourceRange.baseMipLevel = mip;
        imageBarrier.image = image;

        VkDependencyInfo depInfo { .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO, .pNext = nullptr };
        depInfo.imageMemoryBarrierCount = 1;
        depInfo.pImageMemoryBarriers = &imageBarrier;

        vkCmdPipelineBarrier2(cmd, &depInfo);

        if (mip < mipLevels - 1) {
            VkImageBlit2 blitRegion { .sType = VK_STRUCTURE_TYPE_IMAGE_BLIT_2, .pNext = nullptr };

            blitRegion.srcOffsets[1].x = extent.width;
            blitRegion.srcOffsets[1].y = extent.height;
            blitRegion.srcOffsets[1].z = 1;

            blitRegion.dstOffsets[1].x = halfSize.width;
            blitRegion.dstOffsets[1].y = halfSize.height;
            blitRegion.dstOffsets[1].z = 1;

            blitRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            blitRegion.srcSubresource.baseArrayLayer = 0;
            blitRegion.srcSubresource.layerCount = 1;
            blitRegion.srcSubresource.mipLevel = mip;

            blitRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            blitRegion.dstSubresource.baseArrayLayer = 0;
            blitRegion.dstSubresource.layerCount = 1;
            blitRegion.dstSubresource.mipLevel = mip + 1;

            VkBlitImageInfo2 blitInfo {.sType = VK_STRUCTURE_TYPE_BLIT_IMAGE_INFO_2, .pNext = nullptr};
            blitInfo.dstImage = image;
            blitInfo.dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            blitInfo.srcImage = image;
            blitInfo.srcImageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            blitInfo.filter = VK_FILTER_LINEAR;
            blitInfo.regionCount = 1;
            blitInfo.pRegions = &blitRegion;

            vkCmdBlitImage2(cmd, &blitInfo);

            extent = halfSize;
        }
    }

    // transition all mip levels into the final read_only layout
    CVulkanUtils::transitionImage(cmd, image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
}

SImage CResourceManager::allocateImage(void* inData, const uint32& size, const std::string& inDebugName, VkExtent3D inExtent, VkFormat inFormat, VkImageUsageFlags inFlags, VkImageAspectFlags inViewFlags, bool inMipmapped) {
	ZoneScopedAllocation(fmts("Allocate Image {}", inDebugName.c_str()));

	//size_t data_size = inExtent.depth * inExtent.width * inExtent.height * 4;

	// Upload buffer is not needed outside of this function
	CResourceManager manager;
	const SBuffer uploadBuffer = manager.allocateBuffer(size, VMA_MEMORY_USAGE_CPU_TO_GPU, VK_BUFFER_USAGE_TRANSFER_SRC_BIT);

	memcpy(uploadBuffer->GetMappedData(), inData, size);

	SImage new_image = allocateImage(inDebugName, inExtent, inFormat, inFlags | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, inViewFlags, inMipmapped);

	CVulkanRenderer::immediateSubmit([&](VkCommandBuffer cmd) {
		ZoneScopedAllocation(std::string("Copy Image from Upload Buffer"));
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
		vkCmdCopyBufferToImage(cmd, uploadBuffer->buffer, new_image->mImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);

		if (inMipmapped) {
			generateMipmaps(cmd, new_image->mImage, VkExtent2D{new_image->mImageExtent.width,new_image->mImageExtent.height});
		} else {
			CVulkanUtils::transitionImage(cmd, new_image->mImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
			}
		}
	);

	manager.flush();

	return new_image;
}

basisu::image readImage(const std::string& path) {
	ZoneScopedAllocation(std::string("Read Image"));

	basisu::image img;
	basisu::load_png(path.c_str(), img);
	return img;
}

basisu::gpu_image_vec compress(const std::string& path, const basisu::image& image) {
	ZoneScopedAllocation(std::string("Compress Image"));

	size_t file_size = 0;
	constexpr uint32 quality = 255;

	const uint32 width = image.get_width();
	const uint32 height = image.get_height();

	basisu::vector<basisu::image> images;
	images.push_back(image);

	// TODO cETC1S is supremely smaller, but takes significantly longer to compress. cETC1S should be the main format.
	void* pKTX2_data = basisu::basis_compress(
		basist::basis_tex_format::cUASTC4x4,
		images,
		basisu::cFlagGenMipsClamp | basisu::cFlagKTX2 | basisu::cFlagSRGB | basisu::cFlagThreaded | basisu::cFlagUseOpenCL | basisu::cFlagDebug | basisu::cFlagPrintStats | basisu::cFlagPrintStatus, 0.0f,
		&file_size,
		nullptr);//quality |

	if (!pKTX2_data) {
		errs("Compress for file {} failed.", path.c_str());
	}

	basisu::gpu_image_vec vec;
	{
		ZoneScopedAllocation(std::string("Transcode Image"));

		// Create and Initialize the KTX2 transcoder object.
		basist::ktx2_transcoder transcoder;
		if (!transcoder.init(pKTX2_data, file_size)) {
			errs("Basisu transcoder not initialized for file {}.", path.c_str());
		}

		printf("Texture dimensions: %ux%u, levels: %u\n", width, height, transcoder.get_levels());

		transcoder.start_transcoding();

		// Transcode each mipmap and add to vector
		for (uint32 mipmap = 0; mipmap < transcoder.get_levels(); ++mipmap) {
			const uint32_t level_width = basisu::maximum<uint32_t>(width >> mipmap, 1);
			const uint32_t level_height = basisu::maximum<uint32_t>(height >> mipmap, 1);
			basisu::gpu_image tex(basisu::texture_format::cBC7, level_width, level_height); //cDecodeFlagsTranscodeAlphaDataToOpaqueFormats

			if (const bool status = transcoder.transcode_image_level(mipmap, 0, 0, tex.get_ptr(), tex.get_total_blocks(), basist::transcoder_texture_format::cTFBC7_RGBA, 0); !status) {
				errs("Image Transcode for file {} failed.", path.c_str());
			}

			vec.push_back(tex);
		}
	}

	basisu::basis_free_data(pKTX2_data);

	return vec;
}

bool saveImage(const std::string& cachedPath, const basisu::gpu_image_vec& imageVector, const uint32 inHash) {
	ZoneScopedAllocation(std::string("Save Image"));

	basisu::vector<basisu::gpu_image_vec> tex_vec;
	tex_vec.push_back(imageVector);
	if (!basisu::write_dds_file(cachedPath.c_str(), tex_vec, false, true)) {
		return false;
	}

	std::filesystem::path hashPath(cachedPath);
	hashPath.replace_extension(".hsh");

	CFileArchive file(hashPath.string(), "wb");

	const std::vector vector = {inHash};
	file.writeFile(vector);

	return true;
}

// Copied from TinyDDS, since it is implemented in basis, i cant use it here without making my own
// TODO: texture utilities class
uint32_t mipMapReduce(uint32_t value, uint32_t mipmaplevel) {

	// handle 0 being passed in
	if (value <= 1)
		return 1;

	// there are better ways of doing this (log2 etc.) but this doesn't require any
	// dependecies and isn't used enough to matter imho
	for (uint32_t i = 0u; i < mipmaplevel; ++i) {
		if (value <= 1)
			return 1;
		value = value / 2;
	}
	return value;
}

// TODO: other file types (other bcs, cubemap, array)
void CResourceManager::readDDSFile(const char* pFilename) {
	ZoneScopedAllocation(std::string("Read DDS File"));

	constexpr uint32 MAX_IMAGE_DIM = 16384;

	TinyDDS_Callbacks cbs;

	cbs.errorFn = [](void* user, char const* msg) { BASISU_NOTE_UNUSED(user); fprintf(stderr, "tinydds: %s\n", msg); };
	cbs.allocFn = [](void* user, size_t size) -> void* { BASISU_NOTE_UNUSED(user); return malloc(size); };
	cbs.freeFn = [](void* user, void* memory) { BASISU_NOTE_UNUSED(user); free(memory); };
	cbs.readFn = [](void* user, void* buffer, size_t byteCount) -> size_t { return (size_t)fread(buffer, 1, byteCount, (FILE*)user); };

#ifdef _MSC_VER
	cbs.seekFn = [](void* user, int64_t ofs) -> bool { return _fseeki64((FILE*)user, ofs, SEEK_SET) == 0; };
	cbs.tellFn = [](void* user) -> int64_t { return _ftelli64((FILE*)user); };
#else
	cbs.seekFn = [](void* user, int64_t ofs) -> bool { return fseek((FILE*)user, (long)ofs, SEEK_SET) == 0; };
	cbs.tellFn = [](void* user) -> int64_t { return (int64_t)ftell((FILE*)user); };
#endif

	FILE* pFile;
	fopen_s(&pFile, pFilename, "rb");
	if (!pFile) {
		errs("Can't open .DDS file {}", pFilename);
	}

	bool status = false;
	uint32 width = 0, height = 0;

	const TinyDDS_ContextHandle ctx = TinyDDS_CreateContext(&cbs, pFile);
	if (!ctx) {
		errs("Tiny DDS failed to initialize in file {}", pFilename);
	}

	status = TinyDDS_ReadHeader(ctx);
	if (!status) {
		errs("Failed parsing DDS header in file {}", pFilename);
	}

	if (!TinyDDS_Is2D(ctx) || TinyDDS_ArraySlices(ctx) > 1 || TinyDDS_IsCubemap(ctx)) {
		errs("Unsupported DDS texture type in file {}", pFilename);
	}

	width = TinyDDS_Width(ctx);
	height = TinyDDS_Height(ctx);

	if (!width || !height) {
		errs("DDS texture dimensions invalid in file {}", pFilename);
	}

	if (width > MAX_IMAGE_DIM || height > MAX_IMAGE_DIM) {
		errs("DDS texture dimensions too large in file {}", pFilename);
	}

	VkExtent3D imageSize;
	imageSize.width = width;
	imageSize.height = height;
	imageSize.depth = 1;

	const uint32 numMips = TinyDDS_NumberOfMipmaps(ctx);

	VkFormat format;

	//TODO: how to know if using alpha to save on space
	switch (TinyDDS_GetFormat(ctx)) {
		case TDDS_BC1_RGBA_SRGB_BLOCK:
			format = VK_FORMAT_BC1_RGBA_SRGB_BLOCK;
			break;
		case TDDS_BC1_RGBA_UNORM_BLOCK:
			format = VK_FORMAT_BC1_RGBA_UNORM_BLOCK;
			break;
		case TDDS_BC2_SRGB_BLOCK:
			format = VK_FORMAT_BC2_SRGB_BLOCK;
			break;
		case TDDS_BC2_UNORM_BLOCK:
			format = VK_FORMAT_BC2_UNORM_BLOCK;
			break;
		case TDDS_BC3_SRGB_BLOCK:
			format = VK_FORMAT_BC3_SRGB_BLOCK;
			break;
		case TDDS_BC3_UNORM_BLOCK:
			format = VK_FORMAT_BC3_UNORM_BLOCK;
			break;
		case TDDS_BC4_SNORM_BLOCK:
			format = VK_FORMAT_BC4_SNORM_BLOCK;
			break;
		case TDDS_BC4_UNORM_BLOCK:
			format = VK_FORMAT_BC4_UNORM_BLOCK;
			break;
		case TDDS_BC5_SNORM_BLOCK:
			format = VK_FORMAT_BC5_SNORM_BLOCK;
			break;
		case TDDS_BC5_UNORM_BLOCK:
			format = VK_FORMAT_BC5_UNORM_BLOCK;
			break;
		case TDDS_BC6H_SFLOAT_BLOCK:
			format = VK_FORMAT_BC6H_SFLOAT_BLOCK;
			break;
		case TDDS_BC6H_UFLOAT_BLOCK:
			format = VK_FORMAT_BC6H_UFLOAT_BLOCK;
			break;
		case TDDS_BC7_SRGB_BLOCK:
			format = VK_FORMAT_BC7_SRGB_BLOCK;
			break;
		case TDDS_BC7_UNORM_BLOCK:
			format = VK_FORMAT_BC7_UNORM_BLOCK;
			break;
		default:
			astsNoEntry();
	}


	const SImage image = allocateImage(pFilename, imageSize, format, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, VK_IMAGE_ASPECT_COLOR_BIT, numMips);

	CVulkanRenderer::immediateSubmit([&](VkCommandBuffer cmd) {
		CVulkanUtils::transitionImage(cmd, image->mImage, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
	});

	for (uint32 level = 0; level < numMips; level++) {
		const uint32 level_width = mipMapReduce(width, level);
		const uint32 level_height = mipMapReduce(height, level);

		const void* pImage = TinyDDS_ImageRawData(ctx, level);
		const uint32 image_size = TinyDDS_ImageSize(ctx, level);

		VkExtent3D levelSize;
		levelSize.width = level_width;
		levelSize.height = level_height;
		levelSize.depth = 1;

		// Upload buffer is not needed outside of this function
		CResourceManager manager;
		const SBuffer uploadBuffer = manager.allocateBuffer(image_size, VMA_MEMORY_USAGE_CPU_TO_GPU, VK_BUFFER_USAGE_TRANSFER_SRC_BIT);

		memcpy(uploadBuffer->GetMappedData(), pImage, image_size);

		CVulkanRenderer::immediateSubmit([&](VkCommandBuffer cmd) {
			ZoneScopedAllocation(std::string("Copy Image from Upload Buffer"));

			VkBufferImageCopy copyRegion = {};
			copyRegion.bufferOffset = 0;
			copyRegion.bufferRowLength = 0;
			copyRegion.bufferImageHeight = 0;

			copyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			copyRegion.imageSubresource.mipLevel = level;
			copyRegion.imageSubresource.baseArrayLayer = 0;
			copyRegion.imageSubresource.layerCount = 1;
			copyRegion.imageExtent = levelSize;

			// copy the buffer into the image
			vkCmdCopyBufferToImage(cmd, uploadBuffer->buffer, image->mImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);
		});

		manager.flush();
	}

	CVulkanRenderer::immediateSubmit([&](VkCommandBuffer cmd) {
		CVulkanUtils::transitionImage(cmd, image->mImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	});

	TinyDDS_DestroyContext(ctx);
	fclose(pFile);
}

bool CResourceManager::loadLDRImage(const std::string& cachedPath, const char* inFileName, uint32 inHash) {
	ZoneScopedAllocation(std::string("Load LDR Image"));

	if (!std::filesystem::exists(cachedPath)) {
		msgs("File {} could not be found, recompiling.", inFileName);
		return false;
	}

	std::filesystem::path hashPath(cachedPath);
	hashPath.replace_extension(".hsh");

	if (!std::filesystem::exists(hashPath)) {
		msgs("Hash for file {} could not be found, recompiling.", inFileName);
		return false;
	}

	CFileArchive file(hashPath.string(), "rb");

	if (const std::vector<uint32> vector = file.readFile<uint32>(); inHash != vector[0]) {
		msgs("Image file {} has changed, recompiling.", inFileName);
		return false;
	}

	readDDSFile(cachedPath.c_str());
	return true;
}

void CResourceManager::loadImage(const char* inFileName) {
	ZoneScopedAllocation(fmts("Load Image {}", inFileName));

	msgs("Loading image: {}", inFileName);

	const std::string path = SPaths::get().mAssetPath.string() + inFileName;
	std::filesystem::path cachedPath = SPaths::get().mAssetCachePath.string() + inFileName;
	cachedPath.replace_extension(".dds");

	basisu::image image = readImage(path);

	VkExtent3D imagesize;
	imagesize.width = image.get_width();
	imagesize.height = image.get_height();
	imagesize.depth = 1;

	if (image.get_total_pixels() <= 0) {
		errs("Nothing found in Texture file {}!", inFileName);
	}

	std::string str;
	str.resize(image.get_pixels().size_in_bytes());
	memcpy(str.data(), image.get_pixels().get_ptr(), image.get_pixels().size_in_bytes());

	const uint32 Hash = getHash(str);
	if (Hash == 0) {
		errs("Hash from file {} is not valid.", inFileName);
	}

	if (loadLDRImage(cachedPath.string(), inFileName, Hash)) {
		return;
	}

	const basisu::gpu_image_vec& imageVector = compress(path, image);

	if (!saveImage(cachedPath.string(), imageVector, Hash)) {
		errs("File {} failed to save.", inFileName);
	}

	if (!loadLDRImage(cachedPath.string(), inFileName, Hash)) {
		errs("File {} could not load.", inFileName);
	}

	msgs("Image {} Loaded.", inFileName);
}

void CResourceManager::deallocateImage(const SImage_T* inImage) {
	ZoneScopedAllocation(fmts("Deallocate Image {}", inImage->name.c_str()));
	vmaDestroyImage(getAllocator(), inImage->mImage, inImage->mAllocation);
	vkDestroyImageView(CEngine::device(), inImage->mImageView, nullptr);
}
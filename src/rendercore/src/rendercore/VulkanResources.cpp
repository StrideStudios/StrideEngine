#include "rendercore/VulkanResources.h"

#include "rendercore/BindlessResources.h"

#define VMA_IMPLEMENTATION
#include <vma/vk_mem_alloc.h>

#include "glslang/Include/glslang_c_interface.h"
#include "glslang/Public/resource_limits_c.h"

uint32 SShader::compile() {

	glslang_stage_t stage;
	switch (mStage) {
		case EShaderStage::VERTEX:
			stage = GLSLANG_STAGE_VERTEX;
			break;
		case EShaderStage::FRAGMENT:
			stage = GLSLANG_STAGE_FRAGMENT;
			break;
		case EShaderStage::COMPUTE:
			stage = GLSLANG_STAGE_COMPUTE;
			break;
		default:
			return 0;
	}

	const glslang_input_t input = {
		.language = GLSLANG_SOURCE_GLSL,
		.stage = stage,
		.client = GLSLANG_CLIENT_VULKAN,
		.client_version = GLSLANG_TARGET_VULKAN_1_3,
		.target_language = GLSLANG_TARGET_SPV,
		.target_language_version = GLSLANG_TARGET_SPV_1_6,
		.code = mShaderCode.c_str(),
		.default_version = 460,
		.default_profile = GLSLANG_NO_PROFILE,
		.force_default_version_and_profile = false,
		.forward_compatible = false,
		.messages = GLSLANG_MSG_DEFAULT_BIT,
		.resource = glslang_default_resource(),
	};

	glslang_initialize_process();

	glslang_shader_t* shader = glslang_shader_create(&input);

	if (!glslang_shader_preprocess(shader, &input)) {
		errs("Error Processing Shader. Log: {}. Debug Log: {}.",
			glslang_shader_get_info_log(shader),
			glslang_shader_get_info_debug_log(shader));
	}

	if (!glslang_shader_parse(shader, &input)) {
		errs("Error Parsing Shader. Log: {}. Debug Log: {}.",
			glslang_shader_get_info_log(shader),
			glslang_shader_get_info_debug_log(shader));
	}

	glslang_program_t* program = glslang_program_create();
	glslang_program_add_shader(program, shader);

	if (!glslang_program_link(program, GLSLANG_MSG_SPV_RULES_BIT | GLSLANG_MSG_VULKAN_RULES_BIT)) {
		errs("Error Linking Shader. Log: {}. Debug Log: {}.",
			glslang_shader_get_info_log(shader),
			glslang_shader_get_info_debug_log(shader));
	}

	glslang_program_SPIRV_generate(program, input.stage);

	mCompiledShader.resize(glslang_program_SPIRV_get_size(program));
	glslang_program_SPIRV_get(program, mCompiledShader.data());

	if (glslang_program_SPIRV_get_messages(program)) {
		msgs("{}", glslang_program_SPIRV_get_messages(program));
	}

	glslang_program_delete(program);
	glslang_shader_delete(shader);

	return mCompiledShader.size();
}

std::string readShaderFile(const char* inFileName) {
	CFileArchive file(inFileName, "r");

	if (!file.isOpen()) {
		printf("I/O error. Cannot open shader file '%s'\n", inFileName);
		return std::string();
	}

	std::string code = file.readFile(true);

	// Process includes
	while (code.find("#include ") != code.npos)
	{
		const auto pos = code.find("#include ");
		const auto p1 = code.find("\"", pos);
		const auto p2 = code.find("\"", p1 + 1);
		if (p1 == code.npos || p2 == code.npos || p2 <= p1)
		{
			printf("Error while loading shader program: %s\n", code.c_str());
			return std::string();
		}
		const std::string name = code.substr(p1 + 1, p2 - p1 - 1);
		const std::string include = readShaderFile((SPaths::get().mShaderPath.string() + name.c_str()).c_str());
		code.replace(pos, p2-pos+1, include.c_str());
	}

	return code;
}

bool SShader::loadShader(VkDevice inDevice, const char* inFileName, uint32 Hash) {
	CFileArchive file(inFileName, "rb");

	if (!file.isOpen()) {
		return false;
	}

	std::vector<uint32> code = file.readFile<uint32>();

	// The first uint32 value is the hash, if it does not equal the hash for the shader code, it means the shader has changed
	if (code[0] != Hash) {
		msgs("Shader file {} has changed, recompiling.", inFileName);
		return false;
	}

	// Remove the hash so it doesnt mess up the SPIRV shader
	code.erase(code.begin());

	// Create a new shader module, using the buffer we loaded
	VkShaderModuleCreateInfo createInfo {
		.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
		.pNext = nullptr,
		// CodeSize has to be in bytes, so multply the ints in the buffer by size of
		.codeSize = code.size() * sizeof(uint32),
		.pCode = code.data()
	};

	// Check that the creation goes well.
	if (vkCreateShaderModule(inDevice, &createInfo, nullptr, &mModule) != VK_SUCCESS) {
		return false;
	}
	return true;
}

bool SShader::saveShader(const char* inFileName, uint32 Hash) const {
	CFileArchive file(inFileName, "wb");

	// Make sure the file is open
	if (!file.isOpen()) {
		return false;
	}

	std::vector<uint32> data = mCompiledShader;

	// Add the hash to the first part of the shader
	data.insert(data.begin(), Hash);

	file.writeFile(data);

	msgs("Compiled Shader {}.", inFileName);

	return true;
}

SShader::SShader(const char* inFilePath) {
	const std::string fileExtension = std::filesystem::path(inFilePath).extension().string();

	if (fileExtension == ".comp") {
		mStage = EShaderStage::COMPUTE;
	} else if (fileExtension == ".vert") {
		mStage = EShaderStage::VERTEX;
	} else if (fileExtension == ".frag") {
		mStage = EShaderStage::FRAGMENT;
	}

	const std::string path = SPaths::get().mShaderPath.string() + inFilePath;
	const std::string SPIRVpath = path + ".spv";

	// Get the hash of the original source file so we know if it changed
	const auto shaderSource = readShaderFile(path.c_str());

	if (shaderSource.empty()) {
		errs("Nothing found in Shader file {}!", inFilePath);
	}

	const uint32 Hash = getHash(shaderSource);
	if (Hash == 0) {
		errs("Hash from file {} is not valid.", inFilePath);
	}

	// Check for written SPIRV files
	if (loadShader(CRenderer::vkDevice(), SPIRVpath.c_str(), Hash)) {
		return;
	}

	mShaderCode = shaderSource;
	const uint32 result = compile();
	// Save compiled shader
	if (!saveShader(SPIRVpath.c_str(), Hash)) {
		errs("Shader file {} failed to save to {}!", inFilePath, SPIRVpath.c_str());
	}

	// This means the shader didn't compile properly
	if (!result) {
		errs("Shader file {} failed to compile!", inFilePath);
	}

	if (loadShader(CRenderer::vkDevice(), SPIRVpath.c_str(), Hash)) {
		return;
	}

	errs("Shader file {} could not be loaded!", inFilePath);
}

CPipeline::CPipeline(const SPipelineCreateInfo& inCreateInfo, CVertexAttributeArchive& inAttributes, CPipelineLayout* inLayout):
	mLayout(inLayout) {

	// Make viewport state from our stored viewport and scissor.
	// At the moment we won't support multiple viewports or scissors
	//TODO: multiple viewports, although supported on many GPUs, is not all of them, so it should have a alternative
	VkPipelineViewportStateCreateInfo viewportState = {};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.pNext = nullptr;

	viewportState.viewportCount = 1;
	viewportState.scissorCount = 1;

	VkPipelineColorBlendAttachmentState colorBlendAttachment {
		.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT
	};

	auto setBlending =[&]{
		colorBlendAttachment.blendEnable = VK_TRUE;
		colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
		colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
		colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
	};

	switch (inCreateInfo.mBlendMode) {
		case EBlendMode::NONE:
			colorBlendAttachment.blendEnable = VK_FALSE;
			break;
		case EBlendMode::ADDITIVE:
			setBlending();
			colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
			break;
		case EBlendMode::ALPHA_BLEND:
			setBlending();
			colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
			break;
	}

	VkPipelineColorBlendStateCreateInfo colorBlending = {};
	colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlending.pNext = nullptr;

	colorBlending.logicOpEnable = VK_FALSE;
	colorBlending.logicOp = VK_LOGIC_OP_COPY;
	colorBlending.attachmentCount = 1;
	colorBlending.pAttachments = &colorBlendAttachment;

	const VkPipelineVertexInputStateCreateInfo vertexInputInfo = inAttributes.get();

	std::vector state {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR
	};

	/*if (inCreateInfo.mLineWidth != 1.f) {
		//state.push_back(VK_DYNAMIC_STATE_LINE_WIDTH);
	}*/

	VkPipelineDynamicStateCreateInfo dynamicInfo = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
		.dynamicStateCount = (uint32)state.size(),
		.pDynamicStates = state.data()
	};

	VkPipelineRenderingCreateInfo renderingCreateInfo {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
		.colorAttachmentCount = 1,
		.pColorAttachmentFormats = &inCreateInfo.mColorFormat,
		.depthAttachmentFormat = inCreateInfo.mDepthFormat
	};

	std::vector shaderStages = {
		VkPipelineShaderStageCreateInfo {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			.pNext = nullptr,
			.stage = VK_SHADER_STAGE_VERTEX_BIT,
			.module = inCreateInfo.vertexModule,
			.pName = "main"
		}
	};

	// Fragment shader isn't always necessary, but vertex is
	if (inCreateInfo.fragmentModule) {
		shaderStages.push_back(VkPipelineShaderStageCreateInfo {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			.pNext = nullptr,
			.stage = VK_SHADER_STAGE_FRAGMENT_BIT,
			.module = inCreateInfo.fragmentModule,
			.pName = "main"
		});
	}

	VkPipelineInputAssemblyStateCreateInfo inputAssemblyCreateInfo {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
		.topology = inCreateInfo.mTopology,
		.primitiveRestartEnable = VK_FALSE
	};

	VkPipelineRasterizationStateCreateInfo rasterizationCreateInfo {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
		.polygonMode = inCreateInfo.mPolygonMode,
		.cullMode = inCreateInfo.mCullMode,
		.frontFace = inCreateInfo.mFrontFace,
		.lineWidth = inCreateInfo.mLineWidth
	};

	VkPipelineMultisampleStateCreateInfo multisampleCreateInfo {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
	};

	if (inCreateInfo.mUseMultisampling) {
		//TODO: multisampling
	} else {
		multisampleCreateInfo.sampleShadingEnable = VK_FALSE;
		multisampleCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
		multisampleCreateInfo.minSampleShading = 1.0f;
		multisampleCreateInfo.pSampleMask = nullptr;
		multisampleCreateInfo.alphaToCoverageEnable = VK_FALSE;
		multisampleCreateInfo.alphaToOneEnable = VK_FALSE;
	}

	VkPipelineDepthStencilStateCreateInfo depthStencilCreateInfo {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
		.depthBoundsTestEnable = VK_FALSE,
		.stencilTestEnable = VK_FALSE,
		.front = {},
		.back = {},
		.minDepthBounds = 0.f,
		.maxDepthBounds = 1.f
	};

	switch (inCreateInfo.mDepthTestMode) {
		case EDepthTestMode::NORMAL:
			depthStencilCreateInfo.depthTestEnable = VK_TRUE;
			depthStencilCreateInfo.depthWriteEnable = VK_TRUE;
			depthStencilCreateInfo.depthCompareOp = VK_COMPARE_OP_GREATER_OR_EQUAL;
			break;
		case EDepthTestMode::BEHIND:
			depthStencilCreateInfo.depthTestEnable = VK_FALSE;
			depthStencilCreateInfo.depthWriteEnable = VK_FALSE;
			depthStencilCreateInfo.depthCompareOp = VK_COMPARE_OP_ALWAYS;
			break;
		case EDepthTestMode::FRONT:
			depthStencilCreateInfo.depthTestEnable = VK_FALSE;
			depthStencilCreateInfo.depthWriteEnable = VK_FALSE;
			depthStencilCreateInfo.depthCompareOp = VK_COMPARE_OP_NEVER;
			break;
	}

	// Build the actual pipeline
	// We now use all the info structs we have been writing into into this one
	// To create the pipeline
	VkGraphicsPipelineCreateInfo pipelineInfo {
		.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
		.pNext = &renderingCreateInfo,
		.stageCount = (uint32)shaderStages.size(),
		.pStages = shaderStages.data(),
		.pVertexInputState = &vertexInputInfo,
		.pInputAssemblyState = &inputAssemblyCreateInfo,
		.pViewportState = &viewportState,
		.pRasterizationState = &rasterizationCreateInfo,
		.pMultisampleState = &multisampleCreateInfo,
		.pDepthStencilState = &depthStencilCreateInfo,
		.pColorBlendState = &colorBlending,
		.pDynamicState = &dynamicInfo,
		.layout = inLayout->mPipelineLayout
	};

	if (vkCreateGraphicsPipelines(CRenderer::vkDevice(), VK_NULL_HANDLE, 1, &pipelineInfo,nullptr, &mPipeline) != VK_SUCCESS) {
		msgs("Failed to create pipeline!");
	}
}

void CPipeline::bind(VkCommandBuffer cmd, const VkPipelineBindPoint inBindPoint) const {
	vkCmdBindPipeline(cmd, inBindPoint, mPipeline);
}

VkRenderingAttachmentInfo SRenderAttachment::get(const SImage_T* inImage) const {
	VkRenderingAttachmentInfo info {
		.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
		.pNext = nullptr,
		.loadOp = mLoadOp,
		.storeOp = mStoreOp
	};

	if (inImage) {
		info.imageView = inImage->mImageView;
		info.imageLayout = inImage->mLayout;
	}

	switch (mType) {
		case EAttachmentType::COLOR:
			info.clearValue = {
				.color = {
					.float32 = {
						mClearValue[0],
						mClearValue[1],
						mClearValue[2],
						mClearValue[3]
					}
				}
			};
			break;
		case EAttachmentType::DEPTH:
		case EAttachmentType::STENCIL:
			info.clearValue = {
				.depthStencil = {
					.depth = mClearValue[0],
					.stencil = static_cast<uint32>(mClearValue[1])
				}
			};
			break;
	}

	return info;
}

SBuffer_T::SBuffer_T(const uint32 inBufferSize, const VmaMemoryUsage inMemoryUsage, const VkBufferUsageFlags inBufferUsage): mBindlessAddress(0) {
	// allocate buffer
	const VkBufferCreateInfo bufferCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		.pNext = nullptr,
		.size = inBufferSize,
		.usage = inBufferUsage
	};

	const VmaAllocationCreateInfo vmaallocInfo = {
		.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT,
		.usage = inMemoryUsage
	};

	// allocate the buffer
	VK_CHECK(vmaCreateBuffer(SVulkanAllocator::get().getAllocator(), &bufferCreateInfo, &vmaallocInfo, &buffer, &allocation, &info));
}

void SBuffer_T::makeGlobal() {
	// Update descriptors with new buffer
	//TODO: need some way of guaranteeing Buffer addresses so they don't have to be passed in push constants
	static uint32 gCurrentBufferAddress = 0;
	mBindlessAddress = gCurrentBufferAddress;
	gCurrentBufferAddress++;

	updateGlobal();

}

void SBuffer_T::updateGlobal() const {
	//TODO: need some way of guaranteeing Buffer addresses so they don't have to be passed in push constants
	const auto bufferDescriptorInfo = VkDescriptorBufferInfo{
		.buffer = buffer,
		.offset = info.offset,
		.range = info.size
	};

	const auto writeSet = VkWriteDescriptorSet{
		.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		.dstSet = *CBindlessResources::getBindlessDescriptorSet(),
		.dstBinding = gUBOBinding, //TODO: for now UBO bindings
		.dstArrayElement = mBindlessAddress,
		.descriptorCount = 1,
		.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		.pBufferInfo = &bufferDescriptorInfo,
	};
	vkUpdateDescriptorSets(CRenderer::vkDevice(), 1, &writeSet, 0, nullptr);
}

void SBuffer_T::destroy() {
	vmaDestroyBuffer(SVulkanAllocator::get().getAllocator(), buffer, allocation);
}

void* SBuffer_T::getMappedData() const {
	return allocation->GetMappedData();
}

void SBuffer_T::mapData(void** data) const {
	vmaMapMemory(SVulkanAllocator::get().getAllocator(), allocation, data);
}

void SBuffer_T::unMapData() const {
	vmaUnmapMemory(SVulkanAllocator::get().getAllocator(), allocation);
}

SMeshBuffers_T::SMeshBuffers_T(const size_t indicesSize, const size_t verticesSize) {
	CResourceManager::get().create(indexBuffer, indicesSize, VMA_MEMORY_USAGE_GPU_ONLY, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
	CResourceManager::get().create(vertexBuffer, verticesSize, VMA_MEMORY_USAGE_GPU_ONLY, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
}

SImage_T::SImage_T(const std::string& inDebugName, const VkExtent3D inExtent, const VkFormat inFormat, const VkImageUsageFlags inFlags, const VkImageAspectFlags inViewFlags, const uint32 inNumMips):
	mName(inDebugName) {

	mImageInfo = CVulkanInfo::createImageInfo(inFormat, inFlags, inExtent);
	mImageInfo.mipLevels = inNumMips;

	//for the draw image, we want to allocate it from gpu local memory
	VmaAllocationCreateInfo imageAllocationInfo = {
		.usage = VMA_MEMORY_USAGE_GPU_ONLY,
		.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT),
	};

	vmaCreateImage(SVulkanAllocator::get().getAllocator(), &mImageInfo, &imageAllocationInfo, &mImage, &mAllocation, nullptr);

	//build a image-view for the draw image to use for rendering
	mImageViewInfo = CVulkanInfo::createImageViewInfo(inFormat, mImage, inViewFlags);
	mImageViewInfo.subresourceRange.levelCount = mImageInfo.mipLevels;

	VK_CHECK(vkCreateImageView(CRenderer::vkDevice(), &mImageViewInfo, nullptr, &mImageView));

	// Update descriptors with new image
	if ((inFlags & VK_IMAGE_USAGE_SAMPLED_BIT) != 0) { //TODO: VK_IMAGE_USAGE_SAMPLED_BIT is not a permanent solution
		// Set and increment current texture address
		static uint32 gCurrentTextureAddress = 0;
		mBindlessAddress = gCurrentTextureAddress;
		gCurrentTextureAddress++;

		const auto imageDescriptorInfo = VkDescriptorImageInfo{
			.imageView = mImageView,
			.imageLayout = VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL
		};

		const auto writeSet = VkWriteDescriptorSet{
			.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.dstSet = *CBindlessResources::getBindlessDescriptorSet(),
			.dstBinding = gTextureBinding,
			.dstArrayElement = mBindlessAddress,
			.descriptorCount = 1,
			.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
			.pImageInfo = &imageDescriptorInfo,
		};
		vkUpdateDescriptorSets(CRenderer::vkDevice(), 1, &writeSet, 0, nullptr);
	}
}

void generateMipmaps(SCommandBuffer cmd, SImage_T* image) {
	int mipLevels = int(std::floor(std::log2(std::max(image->getExtent().width, image->getExtent().height)))) + 1;
	VkExtent2D extent = {image->getExtent().width, image->getExtent().height};
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
        imageBarrier.image = image->mImage;

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
            blitInfo.dstImage = image->mImage;
            blitInfo.dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            blitInfo.srcImage = image->mImage;
            blitInfo.srcImageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            blitInfo.filter = VK_FILTER_LINEAR;
            blitInfo.regionCount = 1;
            blitInfo.pRegions = &blitRegion;

            vkCmdBlitImage2(cmd, &blitInfo);

            extent = halfSize;
        }
    }

    // transition all mip levels into the final read_only layout
    CVulkanUtils::transitionImage(cmd, image, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
}

void SImage_T::push(const void* inData, const uint32& inSize) {
	//size_t data_size = inExtent.depth * inExtent.width * inExtent.height * 4;

	// Upload buffer is not needed outside of this function
	// TODO: Some way of doing an upload buffer generically
	SStagingBuffer uploadBuffer{inSize}; //TODO: was CPU_TO_GPU, test if errors

	memcpy(uploadBuffer.get()->getMappedData(), inData, inSize);

	CRenderer::get()->immediateSubmit([&](SCommandBuffer& cmd) {
		CVulkanUtils::transitionImage(cmd, this, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

		VkBufferImageCopy copyRegion = {};
		copyRegion.bufferOffset = 0;
		copyRegion.bufferRowLength = 0;
		copyRegion.bufferImageHeight = 0;

		copyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		copyRegion.imageSubresource.mipLevel = 0;
		copyRegion.imageSubresource.baseArrayLayer = 0;
		copyRegion.imageSubresource.layerCount = 1;
		copyRegion.imageExtent = getExtent();

		// copy the buffer into the image
		vkCmdCopyBufferToImage(cmd, uploadBuffer.get()->buffer, mImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);

		if (isMipmapped()) {
			generateMipmaps(cmd, this);
		} else {
			CVulkanUtils::transitionImage(cmd, this, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		}
	});

	uploadBuffer.destroy();
}

void SImage_T::destroy() {
	vmaDestroyImage(SVulkanAllocator::get().getAllocator(), mImage, mAllocation);
	vkDestroyImageView(CRenderer::vkDevice(), mImageView, nullptr);
}

void SVulkanAllocator::init() {
	const VmaAllocatorCreateInfo allocatorInfo {
		.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT,
		.physicalDevice = CRenderer::vkPhysicalDevice(),
		.device = CRenderer::vkDevice(),
		.instance = CRenderer::vkInstance()
	};

	VK_CHECK(vmaCreateAllocator(&allocatorInfo, &mAllocator));
}

void SVulkanAllocator::destroy() {
	vmaDestroyAllocator(mAllocator);
}
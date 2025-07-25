#include "GraphicsRenderer.h"

#include <array>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/packing.hpp>

#include "Engine.h"
#include "ShaderCompiler.h"
#include "EngineBuffers.h"
#include "EngineSettings.h"
#include "EngineTextures.h"
#include "ResourceManager.h"
#include "StaticMesh.h"
#include "VkBootstrap.h"

#define COMMAND_CATEGORY "Camera"
ADD_COMMAND(Vector3f, Translation, {0.f, -5.f, 0.f});
ADD_COMMAND(Vector3f, Rotation, {0.f, 0.f, 0.f});
#undef COMMAND_CATEGORY

VkPipeline CPipelineBuilder::buildPipeline(VkDevice inDevice) const {
	// Make viewport state from our stored viewport and scissor.
	// At the moment we won't support multiple viewports or scissors
	//TODO: multiple viewports, although supported on many GPUs, is not all of them, so it should have a alternative
	VkPipelineViewportStateCreateInfo viewportState = {};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.pNext = nullptr;

	viewportState.viewportCount = 1;
	viewportState.scissorCount = 1;

	// Setup dummy color blending. We aren't using transparent objects yet
	// The blending is just "no blend", but we do write to the color attachment
	VkPipelineColorBlendStateCreateInfo colorBlending = {};
	colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlending.pNext = nullptr;

	colorBlending.logicOpEnable = VK_FALSE;
	colorBlending.logicOp = VK_LOGIC_OP_COPY;
	colorBlending.attachmentCount = 1;
	colorBlending.pAttachments = &m_ColorBlendAttachment;

	// Completely clear VertexInputStateCreateInfo, as we have no need for it
	VkPipelineVertexInputStateCreateInfo _vertexInputInfo = { .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };

	VkDynamicState state[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };

	VkPipelineDynamicStateCreateInfo dynamicInfo = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
		.dynamicStateCount = 2,
		.pDynamicStates = &state[0]
	};

	// Build the actual pipeline
	// We now use all the info structs we have been writing into into this one
	// To create the pipeline
	VkGraphicsPipelineCreateInfo pipelineInfo {
		.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
		.pNext = &m_RenderInfo,

		.stageCount = (uint32_t)m_ShaderStages.size(),
		.pStages = m_ShaderStages.data(),
		.pVertexInputState = &_vertexInputInfo,
		.pInputAssemblyState = &m_InputAssembly,
		.pViewportState = &viewportState,
		.pRasterizationState = &m_Rasterizer,
		.pMultisampleState = &m_Multisampling,
		.pDepthStencilState = &m_DepthStencil,
		.pColorBlendState = &colorBlending,
		.pDynamicState = &dynamicInfo,
		.layout = m_PipelineLayout
	};

	// its easy to error out on create graphics pipeline, so we handle it a bit
	// better than the common VK_CHECK case
	VkPipeline newPipeline;
	if (vkCreateGraphicsPipelines(inDevice, VK_NULL_HANDLE, 1, &pipelineInfo,nullptr, &newPipeline) != VK_SUCCESS) {
		msgs("Failed to create pipeline!");
		return VK_NULL_HANDLE; // failed to create graphics pipeline
	}
	return newPipeline;
}

void CPipelineBuilder::setShaders(VkShaderModule inVertexShader, VkShaderModule inFragmentShader) {
	m_ShaderStages.clear();

	//TODO: CVulkanInfo create
	VkPipelineShaderStageCreateInfo vertexStageInfo {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
		.pNext = nullptr,
		.stage = VK_SHADER_STAGE_VERTEX_BIT,
		.module = inVertexShader,
		.pName = "main"
	};

	VkPipelineShaderStageCreateInfo pixelStageInfo {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
		.pNext = nullptr,
		.stage = VK_SHADER_STAGE_FRAGMENT_BIT,
		.module = inFragmentShader,
		.pName = "main"
	};

	m_ShaderStages.push_back(vertexStageInfo);

	m_ShaderStages.push_back(pixelStageInfo);
}

void CPipelineBuilder::clear() {
	// Clear all the structs we need back to 0 with their correct stype

	m_InputAssembly = { .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };

	m_Rasterizer = { .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };

	m_ColorBlendAttachment = {};

	m_Multisampling = { .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };

	m_PipelineLayout = {};

	m_DepthStencil = { .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };

	m_RenderInfo = { .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO };

	m_ShaderStages.clear();
}

void CGraphicsRenderer::init() {
	CBaseRenderer::init();

	uint32 white = glm::packUnorm4x8(glm::vec4(1, 1, 1, 1));
	m_WhiteImage = mGlobalResourceManager.allocateImage(&white, {1, 1, 1}, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_ASPECT_COLOR_BIT);

	uint32 grey = glm::packUnorm4x8(glm::vec4(0.66f, 0.66f, 0.66f, 1));
	m_GreyImage = mGlobalResourceManager.allocateImage(&grey, {1, 1, 1}, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_ASPECT_COLOR_BIT);

	uint32 black = glm::packUnorm4x8(glm::vec4(0, 0, 0, 0));
	m_BlackImage = mGlobalResourceManager.allocateImage(&white, {1, 1, 1}, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_ASPECT_COLOR_BIT);

	//checkerboard image
	uint32_t magenta = glm::packUnorm4x8(glm::vec4(1, 0, 1, 1));
	std::array<uint32_t, 16 *16 > pixels; //for 16x16 checkerboard texture
	for (int x = 0; x < 16; x++) {
		for (int y = 0; y < 16; y++) {
			pixels[y*16 + x] = ((x % 2) ^ (y % 2)) ? magenta : black;
		}
	}
	m_ErrorCheckerboardImage = mGlobalResourceManager.allocateImage(pixels.data(), VkExtent3D{16, 16, 1}, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_ASPECT_COLOR_BIT);

	VkSamplerCreateInfo sampl = {
		.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO
	};

	sampl.magFilter = VK_FILTER_NEAREST;
	sampl.minFilter = VK_FILTER_NEAREST;

	m_DefaultSamplerNearest = mGlobalResourceManager.allocateSampler(sampl);

	sampl.magFilter = VK_FILTER_LINEAR;
	sampl.minFilter = VK_FILTER_LINEAR;
	m_DefaultSamplerLinear = mGlobalResourceManager.allocateSampler(sampl);

	{
		SDescriptorLayoutBuilder builder;
		builder.addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
		m_SingleImageDescriptorLayout = builder.build(VK_SHADER_STAGE_FRAGMENT_BIT);
	}

	initTrianglePipeline();

	initMeshPipeline();
}

void CGraphicsRenderer::destroy() {
	CBaseRenderer::destroy();
	vkDestroyDescriptorSetLayout(CEngine::device(), m_SingleImageDescriptorLayout, nullptr);
}

void CGraphicsRenderer::initTrianglePipeline() {
	VkDevice device = CEngine::device();

	SShader frag {
		.mStage = EShaderStage::PIXEL
	};
	VK_CHECK(CShaderCompiler::getShader(device,"basic\\colored_triangle.frag", frag));

	SShader vert {
		.mStage = EShaderStage::VERTEX
	};
	VK_CHECK(CShaderCompiler::getShader(device,"basic\\colored_triangle.vert", vert));

	//build the pipeline layout that controls the inputs/outputs of the shader
	//we are not using descriptor sets or other systems yet, so no need to use anything other than empty default
	VkPipelineLayoutCreateInfo layoutCreateInfo {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		.pNext = nullptr,
		.setLayoutCount = 1,
		.pSetLayouts = &m_DrawImageDescriptorLayout
	};

	m_TrianglePipelineLayout = mGlobalResourceManager.allocatePipelineLayout(layoutCreateInfo);

	CPipelineBuilder pipelineBuilder;

	//use the triangle layout we created
	pipelineBuilder.m_PipelineLayout = m_TrianglePipelineLayout;
	//connecting the vertex and pixel shaders to the pipeline
	pipelineBuilder.setShaders(vert.mModule, frag.mModule);
	//it will draw triangles
	pipelineBuilder.setInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
	//filled triangles
	pipelineBuilder.setPolygonMode(VK_POLYGON_MODE_FILL);
	//no backface culling
	pipelineBuilder.setCullMode(VK_CULL_MODE_NONE, VK_FRONT_FACE_CLOCKWISE);
	//no multisampling
	pipelineBuilder.setNoMultisampling();
	//no blending
	pipelineBuilder.disableBlending();
	//no depth testing
	pipelineBuilder.disableDepthTest();

	//connect the image format we will draw into, from draw image
	pipelineBuilder.setColorAttachementFormat(m_EngineTextures->mDrawImage->mImageFormat);
	pipelineBuilder.setDepthFormat(VK_FORMAT_UNDEFINED);

	//finally build the pipeline
	m_TrianglePipeline = mGlobalResourceManager.allocatePipeline(pipelineBuilder);

	//clean structures
	vkDestroyShaderModule(device, frag.mModule, nullptr);
	vkDestroyShaderModule(device, vert.mModule, nullptr);
}

void CGraphicsRenderer::initMeshPipeline() {
	VkDevice device = CEngine::device();

	SShader frag {
		.mStage = EShaderStage::PIXEL
	};
	VK_CHECK(CShaderCompiler::getShader(device,"basic\\tex_image.frag", frag));

	SShader vert {
		.mStage = EShaderStage::VERTEX
	};
	VK_CHECK(CShaderCompiler::getShader(device,"basic\\colored_triangle_mesh.vert", vert));

	VkPushConstantRange bufferRange{
		.stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
		.offset = 0,
		.size = sizeof(SGPUDrawPushConstants)
	};

	//build the pipeline layout that controls the inputs/outputs of the shader
	//we are not using descriptor sets or other systems yet, so no need to use anything other than empty default
	VkPipelineLayoutCreateInfo layoutCreateInfo {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		.pNext = nullptr,
		.setLayoutCount = 1,
		.pSetLayouts = &m_SingleImageDescriptorLayout,
		.pushConstantRangeCount = 1,
		.pPushConstantRanges = &bufferRange
	};

	m_MeshPipelineLayout = mGlobalResourceManager.allocatePipelineLayout(layoutCreateInfo);

	CPipelineBuilder pipelineBuilder;

	//use the triangle layout we created
	pipelineBuilder.m_PipelineLayout = m_MeshPipelineLayout;
	//connecting the vertex and pixel shaders to the pipeline
	pipelineBuilder.setShaders(vert.mModule, frag.mModule);
	//it will draw triangles
	pipelineBuilder.setInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
	//filled triangles
	pipelineBuilder.setPolygonMode(VK_POLYGON_MODE_FILL);
	//no backface culling
	pipelineBuilder.setCullMode(VK_CULL_MODE_NONE, VK_FRONT_FACE_CLOCKWISE);
	//no multisampling
	pipelineBuilder.setNoMultisampling();
	//no blending
	pipelineBuilder.disableBlending();
	//depth testing
	pipelineBuilder.enableDepthTest(true, VK_COMPARE_OP_GREATER_OR_EQUAL);

	//connect the image format we will draw into, from draw image
	pipelineBuilder.setColorAttachementFormat(m_EngineTextures->mDrawImage->mImageFormat);
	pipelineBuilder.setDepthFormat(m_EngineTextures->mDepthImage->mImageFormat);

	//finally build the pipeline
	m_MeshPipeline = mGlobalResourceManager.allocatePipeline(pipelineBuilder);

	//clean structures
	vkDestroyShaderModule(device, frag.mModule, nullptr);
	vkDestroyShaderModule(device, vert.mModule, nullptr);
}

void CGraphicsRenderer::render(VkCommandBuffer cmd) {

	// Render the background
	CBaseRenderer::render(cmd);

	CVulkanUtils::transitionImage(cmd, m_EngineTextures->mDrawImage->mImage, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
	CVulkanUtils::transitionImage(cmd, m_EngineTextures->mDepthImage->mImage, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);

	//begin a render pass  connected to our draw image
	VkRenderingAttachmentInfo colorAttachment = CVulkanUtils::createAttachmentInfo(m_EngineTextures->mDrawImage->mImageView, nullptr, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
	VkRenderingAttachmentInfo depthAttachment = CVulkanUtils::createDepthAttachmentInfo(m_EngineTextures->mDepthImage->mImageView, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);

	VkExtent2D extent {
		m_EngineTextures->mDrawImage->mImageExtent.width,
		m_EngineTextures->mDrawImage->mImageExtent.height
	};

	VkRenderingInfo renderInfo = CVulkanUtils::createRenderingInfo(extent, &colorAttachment, &depthAttachment);
	vkCmdBeginRendering(cmd, &renderInfo);

	//set dynamic viewport and scissor
	VkViewport viewport = {};
	viewport.x = 0;
	viewport.y = 0;
	viewport.width = extent.width;
	viewport.height = extent.height;
	viewport.minDepth = 0.f;
	viewport.maxDepth = 1.f;

	vkCmdSetViewport(cmd, 0, 1, &viewport);

	VkRect2D scissor = {};
	scissor.offset.x = 0;
	scissor.offset.y = 0;
	scissor.extent.width = extent.width;
	scissor.extent.height = extent.height;

	vkCmdSetScissor(cmd, 0, 1, &scissor);

	//renderTrianglePass(cmd);

	renderMeshPass(cmd);

	vkCmdEndRendering(cmd);

	CVulkanUtils::transitionImage(cmd, m_EngineTextures->mDrawImage->mImage,VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

}

void CGraphicsRenderer::renderTrianglePass(VkCommandBuffer cmd) {
	vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_TrianglePipeline);

	//launch a draw command to draw 3 vertices
	vkCmdDraw(cmd, 3, 1, 0, 0);
}

void CGraphicsRenderer::renderMeshPass(VkCommandBuffer cmd) {
	vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_MeshPipeline);

	//bind a texture
	VkDescriptorSet imageSet = getCurrentFrame().mDescriptorAllocator.allocate(m_SingleImageDescriptorLayout);
	{
		SDescriptorWriter writer;
		writer.writeImage(0, m_ErrorCheckerboardImage->mImageView, m_DefaultSamplerNearest, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

		writer.updateSet(imageSet);
	}

	vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_MeshPipelineLayout, 0, 1, &imageSet, 0, nullptr);

	const auto [x, y, z] = m_EngineTextures->mDrawImage->mImageExtent;
	glm::mat4 view = glm::translate(glm::mat4(1.f), { Translation.get().x,Translation.get().z,Translation.get().y }); // Swap y and z so z will be the up vector
	// camera projection
	glm::mat4 projection = glm::perspective(glm::radians(70.f), (float)x / (float)y, 0.1f, 10000.f);

	// invert the Y direction on projection matrix so that we are more similar
	// to opengl and gltf axis
	projection[1][1] *= -1;
	
	SGPUDrawPushConstants push_constants;
	/*push_constants.worldMatrix = Matrix4f::Identity();
	push_constants.vertexBuffer = m_EngineBuffers->mRectangle->vertexBufferAddress;

	vkCmdPushConstants(cmd, m_MeshPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(SGPUDrawPushConstants), &push_constants);
	vkCmdBindIndexBuffer(cmd, m_EngineBuffers->mRectangle->indexBuffer->buffer, 0, VK_INDEX_TYPE_UINT32);

	vkCmdDrawIndexed(cmd, 6, 1, 0, 0, 0);*/

	auto mesh = m_EngineBuffers->testMeshes[0];
	push_constants.worldMatrix = projection * view;
	push_constants.vertexBuffer = mesh->meshBuffers->vertexBufferAddress;

	vkCmdPushConstants(cmd, m_MeshPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(SGPUDrawPushConstants), &push_constants);
	vkCmdBindIndexBuffer(cmd, mesh->meshBuffers->indexBuffer->buffer, 0, VK_INDEX_TYPE_UINT32);

	vkCmdDrawIndexed(cmd, mesh->surfaces[0].count, 1, mesh->surfaces[0].startIndex, 0, 0);

}


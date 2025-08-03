#include "PipelineBuilder.h"

#include <array>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/packing.hpp>

#include "Engine.h"
#include "ShaderCompiler.h"
#include "EngineBuffers.h"
#include "EngineSettings.h"
#include "EngineTextures.h"
#include "GpuScene.h"
#include "ResourceManager.h"
#include "StaticMesh.h"
#include "VkBootstrap.h"
#include "VulkanUtils.h"

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

	constexpr VkVertexInputBindingDescription binding = {
		.binding = 0,
		.stride = sizeof(SVertex),
		.inputRate = VK_VERTEX_INPUT_RATE_VERTEX
	};

	auto attributes = {
		{ // Vector3f position
			0,
			binding.binding,
			VK_FORMAT_R32G32B32_SFLOAT,
			0
		},
		VkVertexInputAttributeDescription{ // float uv_x
			1,
			binding.binding,
			VK_FORMAT_R32_SFLOAT,
			3 * sizeof(float)
		},
		VkVertexInputAttributeDescription{ // Vector3f normal
			2,
			binding.binding,
			VK_FORMAT_R32G32B32_SFLOAT,
			4 * sizeof(float)
		},
		VkVertexInputAttributeDescription{ // float uv_y
			3,
			binding.binding,
			VK_FORMAT_R32_SFLOAT,
			7 * sizeof(float)
		},
		VkVertexInputAttributeDescription{ // Vector4f color
			4,
			binding.binding,
			VK_FORMAT_R32G32B32A32_SFLOAT,
			8 * sizeof(float)
		}
	};

	const VkPipelineVertexInputStateCreateInfo _vertexInputInfo = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
		.vertexBindingDescriptionCount = 1,
		.pVertexBindingDescriptions = &binding,
		.vertexAttributeDescriptionCount = (uint32)attributes.size(),
		.pVertexAttributeDescriptions = attributes.begin()
	};

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

		.stageCount = (uint32)m_ShaderStages.size(),
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

VkPipeline CPipelineBuilder::buildSpritePipeline(VkDevice inDevice) const {
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

		.stageCount = (uint32)m_ShaderStages.size(),
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
#pragma once

#include "BaseRenderer.h"

// push constants for our mesh object draws
struct SGPUDrawPushConstants {
	Matrix4f worldMatrix;
	VkDeviceAddress vertexBuffer;
};

class CPipelineBuilder {
public:
	std::vector<VkPipelineShaderStageCreateInfo> m_ShaderStages;

	VkPipelineInputAssemblyStateCreateInfo m_InputAssembly;
	VkPipelineRasterizationStateCreateInfo m_Rasterizer;
	VkPipelineColorBlendAttachmentState m_ColorBlendAttachment;
	VkPipelineMultisampleStateCreateInfo m_Multisampling;
	VkPipelineLayout m_PipelineLayout;
	VkPipelineDepthStencilStateCreateInfo m_DepthStencil;
	VkPipelineRenderingCreateInfo m_RenderInfo;
	VkFormat m_ColorAttachmentformat;

	CPipelineBuilder(){ clear(); }

	VkPipeline buildPipeline(VkDevice inDevice);

	void setShaders(VkShaderModule inVertexShader, VkShaderModule inFragmentShader);

	void setInputTopology(VkPrimitiveTopology inTopology) {
		m_InputAssembly.topology = inTopology;
		// TODO: Triangle strips and line strips, research
		m_InputAssembly.primitiveRestartEnable = VK_FALSE;
	}

	void setPolygonMode(VkPolygonMode mode) {
		m_Rasterizer.polygonMode = mode;
		m_Rasterizer.lineWidth = 1.f;
	}

	void setCullMode(VkCullModeFlags inCullMode, VkFrontFace inFrontFace) {
		m_Rasterizer.cullMode = inCullMode;
		m_Rasterizer.frontFace = inFrontFace;
	}

	void setNoMultisampling() {
		m_Multisampling.sampleShadingEnable = VK_FALSE;
		// Multisampling defaulted to no multisampling (1 sample per pixel)
		m_Multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
		m_Multisampling.minSampleShading = 1.0f;
		m_Multisampling.pSampleMask = nullptr;
		// No alpha to coverage either
		m_Multisampling.alphaToCoverageEnable = VK_FALSE;
		m_Multisampling.alphaToOneEnable = VK_FALSE;
	}

	void disableBlending() {
		// Default write mask
		m_ColorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		// No blending
		m_ColorBlendAttachment.blendEnable = VK_FALSE;
	}

	void setColorAttachementFormat(VkFormat format) {
		m_ColorAttachmentformat = format;
		// Connect the format to the renderInfo  structure
		m_RenderInfo.colorAttachmentCount = 1; // For deferred, probably wont be used then
		m_RenderInfo.pColorAttachmentFormats = &m_ColorAttachmentformat;
	}

	void setDepthFormat(VkFormat format) {
		m_RenderInfo.depthAttachmentFormat = format;
	}

	void disableDepthTest() {
		m_DepthStencil.depthTestEnable = VK_FALSE;
		m_DepthStencil.depthWriteEnable = VK_FALSE;
		m_DepthStencil.depthCompareOp = VK_COMPARE_OP_NEVER;
		m_DepthStencil.depthBoundsTestEnable = VK_FALSE;
		m_DepthStencil.stencilTestEnable = VK_FALSE;
		m_DepthStencil.front = {};
		m_DepthStencil.back = {};
		m_DepthStencil.minDepthBounds = 0.f;
		m_DepthStencil.maxDepthBounds = 1.f;
	}

	void enableDepthTest(bool depthWriteEnable, VkCompareOp op) {
		m_DepthStencil.depthTestEnable = VK_TRUE;
		m_DepthStencil.depthWriteEnable = depthWriteEnable;
		m_DepthStencil.depthCompareOp = op;
		m_DepthStencil.depthBoundsTestEnable = VK_FALSE;
		m_DepthStencil.stencilTestEnable = VK_FALSE;
		m_DepthStencil.front = {};
		m_DepthStencil.back = {};
		m_DepthStencil.minDepthBounds = 0.f;
		m_DepthStencil.maxDepthBounds = 1.f;
	}

	void enableBlendingAdditive() {
		m_ColorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		m_ColorBlendAttachment.blendEnable = VK_TRUE;
		m_ColorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		m_ColorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
		m_ColorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
		m_ColorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		m_ColorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
		m_ColorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
	}

	//outColor = srcColor * srcColorBlendFactor <op> dstColor * dstColorBlendFactor;
	void enableBlendingAlphaBlend() {
		m_ColorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		m_ColorBlendAttachment.blendEnable = VK_TRUE;
		m_ColorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		m_ColorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		m_ColorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
		m_ColorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		m_ColorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
		m_ColorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
	}

	void clear();

};

class CGraphicsRenderer : public CBaseRenderer {

public:

	CGraphicsRenderer() = default;

	void init() override;

	void destroy() override;

	void render(VkCommandBuffer cmd) override;

	void renderTrianglePass(VkCommandBuffer cmd);

	void renderMeshPass(VkCommandBuffer cmd);

private:

	void initTrianglePipeline();

	void initMeshPipeline();

	VkPipeline m_TrianglePipeline;

	VkPipelineLayout m_TrianglePipelineLayout = nullptr;

	VkPipelineLayout m_MeshPipelineLayout;
	VkPipeline m_MeshPipeline;

	VkDescriptorSetLayout m_SingleImageDescriptorLayout;

	SImage m_WhiteImage;
	SImage m_BlackImage;
	SImage m_GreyImage;
	SImage m_ErrorCheckerboardImage;

	VkSampler m_DefaultSamplerLinear;
	VkSampler m_DefaultSamplerNearest;

};

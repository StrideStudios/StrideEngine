#include "BaseRenderer.h"

#include "Engine.h"
#include "EngineSettings.h"
#include "ShaderCompiler.h"

#define COMMAND_CATEGORY "Visuals"
ADD_COMMAND(int32, UseSky, 0, 0, 1);
ADD_COMMAND(Vector4f, GradientColor0, {1, 0, 0, 1});
ADD_COMMAND(Vector4f, GradientColor1, {0, 0, 1, 1});
ADD_COMMAND(Vector4f, SkyParameters, {0.1, 0.2, 0.4, 0.97});
#undef COMMAND_CATEGORY

CBaseRenderer::CBaseRenderer() {
	VkPipelineLayoutCreateInfo computeLayout {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		.pNext = nullptr,
		.setLayoutCount = 1,
		.pSetLayouts = &m_DrawImageDescriptorLayout
	};

	VkPushConstantRange pushConstant {
		.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
		.offset = 0,
		.size = sizeof(SComputePushConstants)
	};

	computeLayout.pPushConstantRanges = &pushConstant;
	computeLayout.pushConstantRangeCount = 1;

	VK_CHECK(vkCreatePipelineLayout(CEngine::get().getDevice().getDevice(), &computeLayout, nullptr, &m_GradientPipelineLayout));


	SShader gradientShader {
		.mStage = EShaderStage::COMPUTE
	};
	VK_CHECK(CShaderCompiler::getShader(CEngine::get().getDevice().getDevice(), "basic\\gradient.comp", gradientShader));

	SShader skyShader {
		.mStage = EShaderStage::COMPUTE
	};
	VK_CHECK(CShaderCompiler::getShader(CEngine::get().getDevice().getDevice(), "basic\\sky.comp", skyShader));

	VkPipelineShaderStageCreateInfo stageinfo {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
		.pNext = nullptr,
		.stage = VK_SHADER_STAGE_COMPUTE_BIT,
		.module = gradientShader.mModule,
		.pName = "main"
	};

	VkComputePipelineCreateInfo computePipelineCreateInfo {
		.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
		.pNext = nullptr,
		.stage = stageinfo,
		.layout = m_GradientPipelineLayout
	};

	SComputeEffect gradient {
		.name = "gradient",
		.layout = m_GradientPipelineLayout,
		.data = {}
	};

	VK_CHECK(vkCreateComputePipelines(CEngine::get().getDevice().getDevice(),VK_NULL_HANDLE,1,&computePipelineCreateInfo, nullptr, &gradient.pipeline));

	// Change the shader module only to create the sky shader
	computePipelineCreateInfo.stage.module = skyShader.mModule;

	SComputeEffect sky {
		.name = "sky",
		.layout = m_GradientPipelineLayout,
		.data = {}
	};

	VK_CHECK(vkCreateComputePipelines(CEngine::get().getDevice().getDevice(),VK_NULL_HANDLE,1,&computePipelineCreateInfo, nullptr, &sky.pipeline));

	m_BackgroundEffects.push_back(gradient);
	m_BackgroundEffects.push_back(sky);

	vkDestroyShaderModule(CEngine::get().getDevice().getDevice(), gradientShader.mModule, nullptr);
	vkDestroyShaderModule(CEngine::get().getDevice().getDevice(), skyShader.mModule, nullptr);

	m_ResourceDeallocator.append({
		&sky.pipeline,
		&gradient.pipeline,
		&m_GradientPipelineLayout
	});
}

void CBaseRenderer::render(VkCommandBuffer cmd) {
	// bind the gradient drawing compute pipeline
	SComputeEffect& effect = m_BackgroundEffects[UseSky.get()];
	vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, effect.pipeline);

	// bind the descriptor set containing the draw image for the compute pipeline
	vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_GradientPipelineLayout, 0, 1, &m_DrawImageDescriptors, 0, nullptr);

	if (UseSky.get() > 0) {
		effect.data.data1 = SkyParameters.get();
	} else {
		effect.data.data1 = GradientColor0.get();
		effect.data.data2 = GradientColor1.get();
	}

	vkCmdPushConstants(cmd, m_GradientPipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0 ,sizeof(SComputePushConstants), &effect.data);

	// execute the compute pipeline dispatch. We are using 16x16 workgroup size so we need to divide by it
	auto [x, y, z] = m_EngineTextures->mDrawImage.mImageExtent;
	vkCmdDispatch(cmd, std::ceil(x / 16.0), std::ceil(y / 16.0), 1);
}


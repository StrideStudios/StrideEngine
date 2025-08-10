#include "SpritePass.h"

#include "Engine.h"
#include "EngineTextures.h"
#include "GpuScene.h"
#include "PipelineBuilder.h"
#include "ShaderCompiler.h"
#include "VulkanRenderer.h"
#include "VulkanUtils.h"
#include "VulkanDevice.h"
#include "tracy/Tracy.hpp"
#include "Viewport.h"

void CSpritePass::init() {

	CVulkanRenderer& renderer = CEngine::renderer();
	CGPUScene* gpuScene = renderer.mGPUScene;

	SShader frag {
		.mStage = EShaderStage::PIXEL
	};
	VK_CHECK(CShaderCompiler::getShader(CEngine::device(), "material\\sprite.frag", frag));

	SShader vert {
		.mStage = EShaderStage::VERTEX
	};
	VK_CHECK(CShaderCompiler::getShader(CEngine::device(),"material\\sprite.vert", vert))

	auto pushConstants = {
		VkPushConstantRange{
			.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
			.offset = 0,
			.size = sizeof(SPushConstants)
		}
	};

	VkPipelineLayoutCreateInfo layoutCreateInfo {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		.pNext = nullptr,
		.setLayoutCount = 1,
		.pSetLayouts = &CResourceManager::getBindlessDescriptorSetLayout(),
		.pushConstantRangeCount = (uint32)pushConstants.size(),
		.pPushConstantRanges = pushConstants.begin()
	};

	auto newLayout = renderer.mGlobalResourceManager.allocatePipelineLayout(layoutCreateInfo);

    opaquePipeline.layout = newLayout;

	// Set shader modules and standard pipeline
	CPipelineBuilder pipelineBuilder;
	pipelineBuilder.setShaders(vert.mModule, frag.mModule);
	pipelineBuilder.setInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
	pipelineBuilder.setPolygonMode(VK_POLYGON_MODE_FILL);
	pipelineBuilder.setCullMode(VK_CULL_MODE_FRONT_BIT, VK_FRONT_FACE_CLOCKWISE);
	pipelineBuilder.setNoMultisampling();
	pipelineBuilder.disableBlending();
	pipelineBuilder.depthTestAlwaysInFront();

	// Set color and depth format
	pipelineBuilder.setColorAttachementFormat(renderer.mEngineTextures->mDrawImage->mImageFormat);

	pipelineBuilder.setDepthFormat(renderer.mEngineTextures->mDepthImage->mImageFormat);

	pipelineBuilder.m_PipelineLayout = newLayout;

	//opaquePipeline.pipeline = renderer.mGlobalResourceManager.allocatePipeline(pipelineBuilder);

	opaquePipeline.pipeline = pipelineBuilder.buildSpritePipeline(CEngine::device());
	renderer.mGlobalResourceManager.push(opaquePipeline.pipeline);

	vkDestroyShaderModule(CEngine::device(), frag.mModule, nullptr);
	vkDestroyShaderModule(CEngine::device(), vert.mModule, nullptr);
}

void CSpritePass::render(VkCommandBuffer cmd) {
	const CVulkanRenderer& renderer = CEngine::renderer();
	const CGPUScene* scene = renderer.mGPUScene;

	// sort the opaque surfaces by material and mesh
	/*{ //TODO: sorting will be needed, but for z-order
		ZoneScopedN("Sort Draws");
		std::ranges::sort(renderObjects, [&](const std::shared_ptr<SStaticMesh>& lhs, const std::shared_ptr<SStaticMesh>& rhs) {
			return lhs->meshBuffers->indexBuffer->buffer < rhs->meshBuffers->indexBuffer->buffer;
		});
	}*/

	//defined outside of the draw function, this is the state we will try to skip
	SMaterialPipeline* lastPipeline = nullptr;
	std::shared_ptr<CMaterial> lastMaterial = nullptr;

	uint32 drawCallCount = 0;
	uint64 vertexCount = 0;

	auto render = [&](const std::shared_ptr<SSprite>& obj) {
		ZoneScoped;
		ZoneName(obj->name.c_str(), obj->name.size());

		//TODO: auto pipeline rebind (or something)
		// If the materials arent the same, rebind material data
		SMaterialPipeline& pipeline = opaquePipeline;// obj->material->getPipeline(renderer);
		if (obj->material != lastMaterial) {
			// If the pipeline has changed, rebind pipeline data
			if (&pipeline != lastPipeline) {
				lastPipeline = &pipeline;
				CResourceManager::bindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.pipeline);
				CResourceManager::bindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.layout, 0, 1, renderer.mGPUScene->m_Frames[renderer.getFrameIndex()].sceneDescriptor);
				CResourceManager::bindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.layout, 1, 1, CResourceManager::getBindlessDescriptorSet());

				CEngine::get().getViewport().set(cmd);
			}
		}

		vkCmdPushConstants(cmd, pipeline.layout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(SPushConstants), obj->material->mConstants.data());

		vkCmdDraw(cmd, 6, obj->mInstances, 0, 0);

		drawCallCount++;
		vertexCount += 6;
	};

	ZoneScopedN("Sprite Pass");

	/*for (const SStaticMesh& draw : m_MainRenderContext.opaqueSurfaces) {
		render(draw);
	}*/

	for (const auto& obj : objects) {
		render(obj);
	}

	// Set number of drawcalls, vertices, and triangles
	//Drawcalls.setText(fmts("Draw Calls: {}", drawCallCount));
	//Vertices.setText(fmts("Vertices: {}", vertexCount));
	//Triangles.setText(fmts("Triangles: {}", vertexCount / 3));
}

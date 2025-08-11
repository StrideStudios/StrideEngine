#include "SpritePass.h"

#include "Engine.h"
#include "EngineLoader.h"
#include "EngineTextures.h"
#include "PipelineBuilder.h"
#include "ShaderCompiler.h"
#include "Sprite.h"
#include "VulkanRenderer.h"
#include "VulkanUtils.h"
#include "VulkanDevice.h"
#include "tracy/Tracy.hpp"
#include "Viewport.h"
#include "EngineSettings.h"

#define SETTINGS_CATEGORY "Rendering/Sprite Pass"
ADD_TEXT(Sprites, "Sprites: ");
ADD_TEXT(SpriteDrawcalls, "Draw Calls: ");
ADD_TEXT(SpriteVertices, "Vertices: ");
ADD_TEXT(SpriteTriangles, "Triangles: ");
#undef SETTINGS_CATEGORY

void CSpritePass::init() {

	CVulkanRenderer& renderer = CEngine::renderer();

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
		.pSetLayouts = &CVulkanResourceManager::getBindlessDescriptorSetLayout(),
		.pushConstantRangeCount = (uint32)pushConstants.size(),
		.pPushConstantRanges = pushConstants.begin()
	};

	CPipelineLayout* newLayout;
	renderer.mGlobalResourceManager.createDestroyable(newLayout, layoutCreateInfo);

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

	pipelineBuilder.m_PipelineLayout = *newLayout;

	renderer.mGlobalResourceManager.createDestroyable(opaquePipeline.pipeline, pipelineBuilder.buildSpritePipeline(CEngine::device()));
	//TODO: better sprite pipeline handling

	vkDestroyShaderModule(CEngine::device(), frag.mModule, nullptr);
	vkDestroyShaderModule(CEngine::device(), vert.mModule, nullptr);
}

void CSpritePass::render(VkCommandBuffer cmd) {
	CVulkanRenderer& renderer = CEngine::renderer();

	// sort the opaque surfaces by material and mesh
	/*{ //TODO: sorting will be needed, but for z-order
		ZoneScopedN("Sort Draws");
		std::ranges::sort(renderObjects, [&](const std::shared_ptr<SStaticMesh>& lhs, const std::shared_ptr<SStaticMesh>& rhs) {
			return lhs->meshBuffers->indexBuffer->buffer < rhs->meshBuffers->indexBuffer->buffer;
		});
	}*/

	std::map<std::shared_ptr<CMaterial>, std::vector<SInstance>> renderObjects;
	{
		ZoneScopedN("Culling");
		for (const auto& renderable : objects) {
			if (renderable) { //Can do bounds check
				if (renderable && renderable->material) {// && isVisible(renderableObject, renderer.mSceneData.mViewProj)) {
					if (renderObjects.contains(renderable->material)) {
						renderObjects[renderable->material].push_back(SInstance{renderable->getTransformMatrix()});
					} else {
						renderObjects.emplace(renderable->material, std::vector{SInstance{renderable->getTransformMatrix()}});
					}
				}
			}
		}
	}

	// Set number of meshes being drawn
	Sprites.setText(fmts("Sprites: {}", instancers.size()));

	//defined outside of the draw function, this is the state we will try to skip
	SMaterialPipeline* lastPipeline = nullptr;

	uint32 drawCallCount = 0;
	uint64 vertexCount = 0;

	ZoneScopedN("Sprite Pass");

	/*for (const SStaticMesh& draw : m_MainRenderContext.opaqueSurfaces) {
		render(draw);
	}*/

	for (auto& [material, instancer] : instancers) {
		const size_t NumInstances = instancer.instances.size();

		ZoneScoped;
		ZoneName(material->mName.c_str(), material->mName.size());

		VkDeviceSize offset = 0u;
		vkCmdBindVertexBuffers(cmd, 0, 1u, &instancer.get()->buffer, &offset);

		//TODO: auto pipeline rebind (or something)
		SMaterialPipeline* pipeline = &opaquePipeline;// obj->material->getPipeline(renderer);
		// If the pipeline has changed, rebind pipeline data
		if (pipeline != lastPipeline) {
			lastPipeline = pipeline;
			CVulkanResourceManager::bindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, *pipeline->pipeline);
			CVulkanResourceManager::bindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, *pipeline->layout, 0, 1, CVulkanResourceManager::getBindlessDescriptorSet());

			CEngine::get().getViewport().set(cmd);
		}

		vkCmdPushConstants(cmd, *pipeline->layout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(SPushConstants), material->mConstants.data());

		vkCmdDraw(cmd, 6, NumInstances, 0, 0);

		drawCallCount++;
		vertexCount += 6 * NumInstances;
	}

	// Set number of drawcalls, vertices, and triangles
	SpriteDrawcalls.setText(fmts("Draw Calls: {}", drawCallCount));
	SpriteVertices.setText(fmts("Vertices: {}", vertexCount));
	SpriteTriangles.setText(fmts("Triangles: {}", vertexCount / 3));
}

void CSpritePass::push(const std::shared_ptr<CSprite>& inObject) {
	//objects.insert(inObject);
	if (inObject && inObject->material) {// && isVisible(renderableObject, renderer.mSceneData.mViewProj)) {
		const SInstance instance(inObject->getTransformMatrix());
		if (instancers.contains(inObject->material)) {
			instancers[inObject->material].push(instance);
		} else {
			SInstancer instancer;
			instancer.push(instance);
			instancers.emplace(inObject->material, instancer);
		}
	}
}

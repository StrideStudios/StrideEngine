#include "SpritePass.h"

#include "Engine.h"
#include "EngineLoader.h"
#include "EngineTextures.h"
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

	SPipelineCreateInfo createInfo {
		.vertexModule = vert.mModule,
		.fragmentModule = frag.mModule,
		.mDepthTestMode = EDepthTestMode::FRONT,
		.mColorFormat = renderer.mEngineTextures->mDrawImage->mImageFormat,
		.mDepthFormat = renderer.mEngineTextures->mDepthImage->mImageFormat
	};

	CVertexAttributeArchive attributes;
	attributes.createBinding(VK_VERTEX_INPUT_RATE_INSTANCE);
	attributes << VK_FORMAT_R32G32B32A32_SFLOAT;// mat4 Transform
	attributes << VK_FORMAT_R32G32B32A32_SFLOAT;
	attributes << VK_FORMAT_R32G32B32A32_SFLOAT;
	attributes << VK_FORMAT_R32G32B32A32_SFLOAT;

	opaquePipeline = renderer.mGlobalResourceManager.allocatePipeline(createInfo, attributes, CVulkanResourceManager::getBasicPipelineLayout());

	vkDestroyShaderModule(CEngine::device(), frag.mModule, nullptr);
	vkDestroyShaderModule(CEngine::device(), vert.mModule, nullptr);
}

void CSpritePass::render(VkCommandBuffer cmd) {
	ZoneScopedN("Sprite Pass");

	//TODO: Z order sorting

	// Set number of sprites being drawn
	Sprites.setText(fmts("Sprites: {}", objects.size()));

	// Defined outside the draw function, this is the state we will try to skip
	CPipeline* lastPipeline = nullptr;

	uint32 drawCallCount = 0;
	uint64 vertexCount = 0;

	for (auto& sprite : objects) {
		SInstancer& instancer = sprite->getInstancer();
		const size_t NumInstances = instancer.instances.size();

		ZoneScoped;
		ZoneName(sprite->mName.c_str(), sprite->mName.size());

		VkDeviceSize offset = 0u;
		vkCmdBindVertexBuffers(cmd, 0, 1u, &instancer.get(sprite->getTransformMatrix())->buffer, &offset);

		//TODO: auto pipeline rebind (or something)
		CPipeline* pipeline = opaquePipeline;// obj->material->getPipeline(renderer);
		// If the pipeline has changed, rebind pipeline data
		if (pipeline != lastPipeline) {
			lastPipeline = pipeline;
			CVulkanResourceManager::bindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->mPipeline);
			CVulkanResourceManager::bindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->mLayout, 0, 1, CVulkanResourceManager::getBindlessDescriptorSet());

			CEngine::get().getViewport().set(cmd);
		}

		vkCmdPushConstants(cmd, pipeline->mLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(SPushConstants), sprite->material->mConstants.data());

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
	objects.insert(inObject);
	/*if (inObject && inObject->material) {// && isVisible(renderableObject, renderer.mSceneData.mViewProj)) {
		const SInstance instance(inObject->getTransformMatrix());
		if (instancers.contains(inObject->material)) {
			instancers[inObject->material].push(instance);
		} else {
			SInstancer instancer;
			instancer.push(instance);
			instancers.emplace(inObject->material, instancer);
		}
	}*/
}

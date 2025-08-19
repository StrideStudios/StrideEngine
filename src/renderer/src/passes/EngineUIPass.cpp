#include "passes/EngineUIPass.h"

#include "renderer/EngineLoader.h"
#include "renderer/EngineTextures.h"
#include "renderer/VulkanRenderer.h"
#include "renderer/VulkanDevice.h"
#include "tracy/Tracy.hpp"
#include "EngineSettings.h"

/*#define SETTINGS_CATEGORY "Rendering/Sprite Pass"
ADD_TEXT(Sprites, "Sprites: ");
ADD_TEXT(SpriteDrawcalls, "Draw Calls: ");
ADD_TEXT(SpriteVertices, "Vertices: ");
ADD_TEXT(SpriteTriangles, "Triangles: ");
#undef SETTINGS_CATEGORY*/

void CEngineUIPass::init() {

	CVulkanRenderer& renderer = *CVulkanRenderer::get();

	CVulkanResourceManager manager;

	const SShader frag = manager.getShader("material\\sprite.frag");

	const SShader vert = manager.getShader("material\\sprite.vert");

	SPipelineCreateInfo createInfo {
		.vertexModule = *vert.mModule,
		.fragmentModule = *frag.mModule,
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

	manager.flush();
}

void CEngineUIPass::render(VkCommandBuffer cmd) {
	ZoneScopedN("Sprite Pass");

	//TODO: Z order sorting

	// Set number of sprites being drawn
	//Sprites.setText(fmts("Sprites: {}", objects.size()));

	// Defined outside the draw function, this is the state we will try to skip
	uint32 drawCallCount = 0;
	uint64 vertexCount = 0;

	/*for (auto& sprite : objects) {
		SInstancer& instancer = sprite->getInstancer();
		const size_t NumInstances = instancer.instances.size();

		ZoneScoped;
		ZoneName(sprite->mName.c_str(), sprite->mName.size());

		VkDeviceSize offset = 0u;
		vkCmdBindVertexBuffers(cmd, 0, 1u, &instancer.get(sprite->getTransformMatrix())->buffer, &offset);

		//TODO: auto pipeline rebind (or something)
		bindPipeline(cmd, opaquePipeline, sprite->material->mConstants);

		vkCmdDraw(cmd, 6, NumInstances, 0, 0);

		drawCallCount++;
		vertexCount += 6 * NumInstances;
	}*/

	// Set number of drawcalls, vertices, and triangles
	/*SpriteDrawcalls.setText(fmts("Draw Calls: {}", drawCallCount));
	SpriteVertices.setText(fmts("Vertices: {}", vertexCount));
	SpriteTriangles.setText(fmts("Triangles: {}", vertexCount / 3));*/
}
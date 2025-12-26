#include "renderer/passes/SpritePass.h"

#include "rendercore/BindlessResources.h"
#include "rendercore/EngineLoader.h"
#include "renderer/EngineTextures.h"
#include "renderer/VulkanRenderer.h"
#include "basic/Profiling.h"
#include "engine/EngineSettings.h"
#include "tracy/Tracy.hpp"
#include "scene/viewport/Sprite.h"

#define SETTINGS_CATEGORY "Rendering/Sprite Pass"
ADD_TEXT(Sprites, "Sprites: ");
ADD_TEXT(SpriteDrawcalls, "Draw Calls: ");
ADD_TEXT(SpriteVertices, "Vertices: ");
ADD_TEXT(SpriteTriangles, "Triangles: ");
#undef SETTINGS_CATEGORY

void CSpritePass::init(TShared<CRenderer> inRenderer) {
	CPass::init(inRenderer);

	const TShared<CVulkanRenderer> renderer = inRenderer.staticCast<CVulkanRenderer>();

	CResourceManager manager;

	SShader* frag;
	manager.create<SShader>(frag, inRenderer->device(), "material\\sprite.frag");

	SShader* vert;
	manager.create<SShader>(vert, inRenderer->device(), "material\\sprite.vert");

	const SPipelineCreateInfo createInfo {
		.vertexModule = vert->mModule,
		.fragmentModule = frag->mModule,
		.mDepthTestMode = EDepthTestMode::FRONT,
		.mColorFormat = renderer->mEngineTextures->mDrawImage->getFormat(),
		.mDepthFormat = renderer->mEngineTextures->mDepthImage->getFormat()
	};

	CVertexAttributeArchive attributes;
	attributes.createBinding(VK_VERTEX_INPUT_RATE_INSTANCE);
	attributes << VK_FORMAT_R32G32B32A32_SFLOAT;// mat4 Transform
	attributes << VK_FORMAT_R32G32B32A32_SFLOAT;
	attributes << VK_FORMAT_R32G32B32A32_SFLOAT;
	attributes << VK_FORMAT_R32G32B32A32_SFLOAT;

	CResourceManager::get().create<CPipeline>(opaquePipeline, inRenderer->device(), createInfo, attributes, CBindlessResources::getBasicPipelineLayout());

	manager.flush();
}

void CSpritePass::render(const SRendererInfo& info, VkCommandBuffer cmd) {
	ZoneScopedN("Sprite Pass");

	//TODO: Z order sorting

	// Set number of sprites being drawn
	Sprites.setText(fmts("Sprites: {}", objects.size()));

	uint32 drawCallCount = 0;
	uint64 vertexCount = 0;

	for (auto& sprite : objects) {
		IInstancer& instancer = sprite->getInstancer();
		const size_t NumInstances = instancer.getNumberOfInstances();

		ZoneScoped;
		ZoneName(sprite->mName.c_str(), sprite->mName.size());

		SRenderStack2f stack;
		stack.push(sprite->getTransformMatrix());

		VkDeviceSize offset = 0u;
		vkCmdBindVertexBuffers(cmd, 0, 1u, &instancer.get(info.allocator, stack)->buffer, &offset);

		stack.pop();

		bindPipeline(cmd, opaquePipeline, sprite->getMaterial()->mConstants);

		vkCmdDraw(cmd, 6, NumInstances, 0, 0);

		drawCallCount++;
		vertexCount += 6 * NumInstances;
	}

	// Set number of drawcalls, vertices, and triangles
	SpriteDrawcalls.setText(fmts("Draw Calls: {}", drawCallCount));
	SpriteVertices.setText(fmts("Vertices: {}", vertexCount));
	SpriteTriangles.setText(fmts("Triangles: {}", vertexCount / 3));
}

void CSpritePass::update() {
	/*for (auto& sprite : objects) {
		IInstancer& instancer = sprite->getInstancer();
		instancer.setDirty();
	}*/
}

void CSpritePass::destroy() {
	/*for (auto& sprite : objects) {
		SInstancer& instancer = sprite->getInstancer();
		instancer.destroy();
	}*/ //TODO: objects.clear() is probably not needed.
	objects.clear();
}

#include "EditorRenderer.h"

#include <memory>
#include <random>

#include "BindlessResources.h"
#include "Material.h"
#include "passes/MeshPass.h"
#include "viewport/Sprite.h"
#include "passes/SpritePass.h"
#include "renderer/EngineTextures.h"
#include "renderer/EngineUI.h"
#include "tracy/Tracy.hpp"
#include "viewport/generic/Text.h"

void CEditorSpritePass::init() {
	CSpritePass::init();

	/*{
		const auto sprite = std::make_shared<CInstancedSprite>();
		sprite->mName = fmts("Instanced Sprite");
		sprite->material = CVulkanRenderer::get()->mEngineTextures->mErrorMaterial;

		Transform2f transform;
		transform.setPosition(Vector2f{0.f});
		//transform.setScale(Vector2f{384.f, 1080.f});
		transform.setScale(Vector2f{0.2f, 1.0f});
		sprite->addInstance(transform);

		push(sprite);
	}*/

	{
		const auto textSprite = std::make_shared<CTextSprite>();
		textSprite->mName = fmts("Text Sprite");
		textSprite->material = CVulkanRenderer::get()->mEngineTextures->mErrorMaterial;

		textSprite->setPosition(Vector2f{0.f});
		//textSprite->setScale(Vector2f{1.f, 1.f});

		push(textSprite);
	}

	CVulkanRenderer& renderer = *CVulkanRenderer::get();

	CResourceManager manager;

	SShader* frag;
	manager.create<SShader>(frag, "material\\text.frag");

	SShader* vert;
	manager.create<SShader>(vert, "material\\text.vert");

	const SPipelineCreateInfo createInfo {
		.vertexModule = vert->mModule,
		.fragmentModule = frag->mModule,
		.mBlendMode = EBlendMode::ALPHA_BLEND,
		.mDepthTestMode = EDepthTestMode::FRONT,
		.mColorFormat = renderer.mEngineTextures->mDrawImage->getFormat(),
		.mDepthFormat = renderer.mEngineTextures->mDepthImage->getFormat()
	};

	CVertexAttributeArchive attributes;
	attributes.createBinding(VK_VERTEX_INPUT_RATE_INSTANCE);
	attributes << VK_FORMAT_R32G32_SFLOAT;// vec2 UV0 //TODO: might be better as vertex input
	attributes << VK_FORMAT_R32G32_SFLOAT;// vec2 UV1
	attributes << VK_FORMAT_R32G32B32A32_SFLOAT;// mat4 Transform
	attributes << VK_FORMAT_R32G32B32A32_SFLOAT;
	attributes << VK_FORMAT_R32G32B32A32_SFLOAT;
	attributes << VK_FORMAT_R32G32B32A32_SFLOAT;

	CResourceManager::get().create<CPipeline>(textPipeline, createInfo, attributes, CBindlessResources::getBasicPipelineLayout());

	manager.flush();

	const auto textMaterial = std::make_shared<CMaterial>();
	textMaterial->mShouldSave = false;
	textMaterial->mName = "Error";
	textMaterial->mPassType = EMaterialPass::ERROR;

	const auto exampleText = std::make_shared<CTextSprite>("Example Text");
	exampleText->mName = "Example Text";
	exampleText->material = textMaterial;
}

static CResourceManager gTextManager;

void CEditorSpritePass::render(VkCommandBuffer cmd) {
	ZoneScopedN("Editor Sprite Pass");

	uint32 drawCallCount = 0;
	uint64 vertexCount = 0;

	for (auto& sprite : objects) {
		size_t NumInstances;

		ZoneScoped;
		ZoneName(sprite->mName.c_str(), sprite->mName.size());

		vkDeviceWaitIdle(CRenderer::vkDevice());

		//TODO: better buffer management so no vkDeviceWait is needed while rendering
		gTextManager.flush();

		if (auto textSprite = std::dynamic_pointer_cast<CTextSprite>(sprite); textSprite) {
			if (CEngineLoader::getFonts().empty()) continue;
			NumInstances = textSprite->getText().size();

			uint32 bufferSize = NumInstances * (sizeof(Vector4f) + sizeof(SInstance));

			SDynamicBuffer<VMA_MEMORY_USAGE_CPU_TO_GPU, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT> buffer{gTextManager};
			buffer.allocate(bufferSize);

			struct SData {
				Vector4f uv;
				SInstance instance;
			};

			std::shared_ptr<CFont> font = CEngineLoader::getFonts().begin()->second;

			std::vector<SData> datas;

			font->forEachLetter(textSprite->getText(), [&datas, &font](const Vector2f& pos, const Vector2f& uv0, const Vector2f& uv1) {
				Transform2f t;
				t.setPosition(pos);
				t.setScale(glm::abs(uv1 - uv0) * Vector2f(font->mAtlasSize));
				datas.push_back({
					.uv = Vector4f{uv0, uv1},
					.instance = {
						.Transform = t.toMatrix()
					}
				});
			});

			memcpy(buffer.get()->getMappedData(), datas.data(), bufferSize);

			VkDeviceSize offset = 0u;
			vkCmdBindVertexBuffers(cmd, 0, 1u, &buffer.get()->buffer, &offset);

			SPushConstants constants = sprite->material->mConstants;
			constants[0].x = font->mAtlasImage->mBindlessAddress;

			bindPipeline(cmd, textPipeline, constants);
		} else {
			IInstancer& instancer = sprite->getInstancer();
			NumInstances = instancer.getNumberOfInstances();

			VkDeviceSize offset = 0u;
			vkCmdBindVertexBuffers(cmd, 0, 1u, &instancer.get(sprite->getTransformMatrix())->buffer, &offset);

			bindPipeline(cmd, opaquePipeline, sprite->material->mConstants);
		}

		vkCmdDraw(cmd, 6, NumInstances, 0, 0);

		drawCallCount++;
		vertexCount += 6 * NumInstances;
	}

	//gTextManager.flush();
}

void CEditorSpritePass::destroy() {
	CSpritePass::destroy();
	gTextManager.flush();
}

void CEditorRenderer::init() {

	CVulkanRenderer::init();

	addPasses(
		&CMeshPass::get(),
		&CSpritePass::get(),
		//&CEditorSpritePass::get(),
		&CEngineUIPass::get()
	);

	constexpr int32 numSprites = 250;

	std::random_device rd;
	std::mt19937 gen(rd());

	std::uniform_int_distribution distribx(0, 100);
	std::uniform_int_distribution distriby(0, 100);
	std::uniform_int_distribution distribr(0, 360);

	const auto sprite = std::make_shared<CInstancedSprite>();
	sprite->mName = fmts("Instanced Sprite");
	sprite->material = mEngineTextures->mErrorMaterial;

	for (int32 i = 0; i < numSprites; ++i) {
		Transform2f transform;
		transform.setPosition(Vector2f{(float)distribx(gen) / 100.f, (float)distriby(gen) / 100.f});
		transform.setScale(Vector2f{0.025f, 0.05f});
		//transform.setScale(Vector2f{50.f, 50.f});
		transform.setRotation((float)distribr(gen));
		sprite->addInstance(transform);
	}

	CSpritePass::get().push(sprite);
}

void CEditorRenderer::destroy() {
	CVulkanRenderer::destroy();
}

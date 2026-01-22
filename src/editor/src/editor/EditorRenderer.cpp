#include "editor/EditorRenderer.h"

#include <memory>
#include <random>

#include "VRI/BindlessResources.h"
#include "rendercore/Font.h"
#include "rendercore/Material.h"
#include "renderer/passes/MeshPass.h"
#include "scene/viewport/Sprite.h"
#include "renderer/passes/SpritePass.h"
#include "renderer/EngineTextures.h"
#include "renderer/passes/EngineUIPass.h"
#include "tracy/Tracy.hpp"
#include "scene/viewport/generic/Text.h"
#include "VRI/VRICommands.h"

void CEditorSpritePass::init(TFrail<CRenderer> inRenderer) {
	CSpritePass::init(inRenderer);

	const TFrail<CVulkanRenderer> vulkanRenderer = inRenderer.staticCast<CVulkanRenderer>();

	/*{
		const auto sprite = std::make_shared<CInstancedSprite>();
		sprite->mName = fmts("Instanced Sprite");
		sprite->material = vulkanRenderer->mEngineTextures->mErrorMaterial;

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
		textSprite->material = vulkanRenderer->mEngineTextures->mErrorMaterial.get();

		textSprite->setPosition(Vector2f{0.f});
		//textSprite->setScale(Vector2f{1.f, 1.f});

		push(textSprite);
	}

	TUnique<SShader> frag{"material\\text.frag"};

	TUnique<SShader> vert{"material\\text.vert"};

	const SPipelineCreateInfo createInfo {
		.vertexModule = vert->mModule,
		.fragmentModule = frag->mModule,
		.mBlendMode = EBlendMode::ALPHA_BLEND,
		.mDepthTestMode = EDepthTestMode::FRONT,
		.mColorFormat = vulkanRenderer->mEngineTextures->mDrawImage->getFormat(),
		.mDepthFormat = vulkanRenderer->mEngineTextures->mDepthImage->getFormat()
	};

	CVertexAttributeArchive attributes;
	attributes.createBinding(VK_VERTEX_INPUT_RATE_INSTANCE);
	attributes << VK_FORMAT_R32G32_SFLOAT;// vec2 UV0 //TODO: might be better as vertex input
	attributes << VK_FORMAT_R32G32_SFLOAT;// vec2 UV1
	attributes << VK_FORMAT_R32G32B32A32_SFLOAT;// mat4 Transform
	attributes << VK_FORMAT_R32G32B32A32_SFLOAT;
	attributes << VK_FORMAT_R32G32B32A32_SFLOAT;
	attributes << VK_FORMAT_R32G32B32A32_SFLOAT;

	textPipeline = TUnique<SPipeline>{createInfo, attributes, CBindlessResources::getBasicPipelineLayout()};

	vert.destroy();
	frag.destroy();

	textMaterial = TUnique<CMaterial>{};
	textMaterial->mShouldSave = false;
	textMaterial->mName = "Error";
	textMaterial->mPassType = EMaterialPass::ERROR;

	const auto exampleText = std::make_shared<CTextSprite>("Example Text");
	exampleText->mName = "Example Text";
	exampleText->material = textMaterial.get();
}

void CEditorSpritePass::render(const SRendererInfo& info, const TFrail<CVRICommands>& cmd) {
	ZoneScopedN("Editor Sprite Pass");

	uint32 drawCallCount = 0;
	uint64 vertexCount = 0;

	for (auto& sprite : objects) {
		size_t NumInstances;

		ZoneScoped;
		ZoneName(sprite->mName.c_str(), sprite->mName.size());

		vkDeviceWaitIdle(CVRI::get()->getDevice()->device);

		if (auto textSprite = std::dynamic_pointer_cast<CTextSprite>(sprite); textSprite) {
			if (CEngineLoader::getFonts().empty()) continue;
			NumInstances = textSprite->getText().size();

			uint32 bufferSize = NumInstances * (sizeof(Vector4f) + sizeof(SInstance));

			struct SData {
				Vector4f uv;
				SInstance instance;
			};

			SFont& font = CEngineLoader::getFonts().begin()->second;

			std::vector<SData> datas;

			font.forEachLetter(textSprite->getText(), [&datas, &font](const Vector2f& pos, const Vector2f& uv0, const Vector2f& uv1) {
				Transform2f t;
				t.setPosition(pos);
				t.setScale(glm::abs(uv1 - uv0) * Vector2f(font.mAtlasSize));
				datas.push_back({
					.uv = Vector4f{uv0, uv1},
					.instance = {
						.Transform = t.toMatrix()
					}
				});
			});

			tempTextBuffer.push(datas.data(), bufferSize);

			VkDeviceSize offset = 0u;
			cmd->bindVertexBuffers(0, 1u, &tempTextBuffer.get()->buffer, &offset);

			SPushConstants constants = sprite->getMaterial()->mConstants;
			constants[0].x = font.mAtlasImage->mBindlessAddress;

			bindPipeline(cmd, textPipeline.get(), constants);
		} else {
			IInstancer& instancer = sprite->getInstancer();
			NumInstances = instancer.getNumberOfInstances();

			SRenderStack2f stack;
			stack.push(sprite->getTransformMatrix());

			VkDeviceSize offset = 0u;
			cmd->bindVertexBuffers(0, 1u, &instancer.get(stack)->buffer, &offset);

			stack.pop();

			bindPipeline(cmd, opaquePipeline.get(), sprite->getMaterial()->mConstants);
		}

		cmd->draw(6, NumInstances, 0, 0);

		drawCallCount++;
		vertexCount += 6 * NumInstances;
	}
}

void CEditorSpritePass::destroy() {
	CSpritePass::destroy();
	tempTextBuffer.destroy();
	textPipeline.destroy();
}

void CEditorRenderer::init() {

	CVulkanRenderer::init();

	addPasses<
		//CMeshPass,
		//CSpritePass,
		//CEditorSpritePass,
		//CEngineUIPass
	>();

	if (CSpritePass* pass = getPass<CSpritePass>()) {
		{
			constexpr int32 numSprites = 250;

			std::random_device rd;
			std::mt19937 gen(rd());

			std::uniform_int_distribution distribx(0, 100);
			std::uniform_int_distribution distriby(0, 100);
			std::uniform_int_distribution distribr(0, 360);

			const auto sprite = std::make_shared<CInstancedSprite>();
		   sprite->mName = fmts("Instanced Sprite");
		   sprite->material = mEngineTextures->mErrorMaterial.get();

		   for (int32 i = 0; i < numSprites; ++i) {
			   Transform2f transform;
			   transform.setPosition(Vector2f{(float)distribx(gen) / 100.f, (float)distriby(gen) / 100.f});
			   transform.setScale(Vector2f{0.025f, 0.05f});
			   //transform.setScale(Vector2f{50.f, 50.f});
			   transform.setRotation((float)distribr(gen));
			   sprite->addInstance(transform);
		   }

		   pass->push(sprite);
		}

		/*{
			const auto sprite = std::make_shared<CSprite>();
			sprite->mName = fmts("Sprite");
			sprite->material = mEngineTextures->mErrorMaterial;

			sprite->setPosition(Vector2f{0.5f, 0.5f});
			sprite->setScale({0.025f, 0.05f});
			sprite->setRotation(45.f);

			pass->push(sprite);
		}*/
	}
}

void CEditorRenderer::destroy() {
	CVulkanRenderer::destroy();
}

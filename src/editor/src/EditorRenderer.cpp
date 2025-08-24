#include "EditorRenderer.h"

#include <memory>
#include <random>

#include "Material.h"
#include "Sprite.h"
#include "passes/SpritePass.h"
#include "renderer/EngineTextures.h"

void CEditorRenderer::init() {
	CVulkanRenderer::init();

	auto material = std::make_shared<CMaterial>();
	material->mShouldSave = false;
	material->mName = "Test";
	material->mPassType = EMaterialPass::OPAQUE;

	constexpr int32 numSprites = 1000;

	std::random_device rd;
	std::mt19937 gen(rd());

	std::uniform_int_distribution distribx(0, 100);
	std::uniform_int_distribution distriby(0, 100);

	const auto sprite = std::make_shared<CInstancedSprite>();
	sprite->mName = fmts("Instanced Sprite");
	sprite->material = mEngineTextures->mErrorMaterial;

	for (int32 i = 0; i < numSprites; ++i) {
		Transform2f transform;
		transform.mPosition = {(float)distribx(gen) / 100.f, (float)distriby(gen) / 100.f};
		transform.mScale = {0.025f, 0.05f};
		sprite->addInstance(transform);
	}

	if (const auto spritePass = getPass<CSpritePass>()) {
		spritePass->push(sprite);
	}

	/*Transform2f transform;
	transform.mOrigin = {0.5f, 0.5f};
	transform.mPosition = {0.5f, 0.5f};
	transform.mScale = {0.5f, 1.0f};
	sprite->addInstance(transform);*/
}

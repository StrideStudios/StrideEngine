#include "EditorRenderer.h"

#include <memory>
#include <random>

#include "Material.h"
#include "viewport/Sprite.h"
#include "passes/SpritePass.h"
#include "renderer/EngineTextures.h"

void CEditorRenderer::init() {
	CVulkanRenderer::init();

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

	if (const auto spritePass = getPass<CSpritePass>()) {
		spritePass->push(sprite);
	}
}

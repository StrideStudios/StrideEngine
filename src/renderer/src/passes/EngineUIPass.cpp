#include "passes/EngineUIPass.h"

#include "renderer/EngineTextures.h"
#include "renderer/VulkanRenderer.h"
#include "renderer/VulkanDevice.h"
#include "viewport/Sprite.h"

void CEngineUIPass::init() {
	CSpritePass::init();

	const auto sprite = std::make_shared<CInstancedSprite>();
	sprite->mName = fmts("Instanced Sprite");
	sprite->material = CVulkanRenderer::get()->mEngineTextures->mErrorMaterial;

	Transform2f transform;
	transform.setPosition(Vector2f{0.f});
	//transform.setScale(Vector2f{384.f, 1080.f});
	transform.setScale(Vector2f{0.2f, 1.0f});
	sprite->addInstance(transform);

	push(sprite);
}
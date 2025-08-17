#include "Engine.h"
#include "Scene.h"

//TODO: required for vulkan module to operate properly (is not a good solution)
#include "renderer/VulkanRenderer.h"

int main() {

	CEngine::get().init();

	CScene::get().init();

	CEngine::get().run();

	CScene::get().destroy();

	CEngine::get().end();

	return 0;
}

#include "Engine.h"
#include "Scene.h"
#include "VulkanRenderer.h"

int main() {

	//CVulkanRendererSection::Registry reg{};

	CEngine::get().init();

	CScene::get().init();

	CEngine::get().run();

	CScene::get().destroy();

	CEngine::get().end();

	return 0;
}

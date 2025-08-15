#include "Engine.h"
#include "VulkanRenderer.h"

int main() {

	//CVulkanRendererSection::Registry reg{};

	CEngine::get().init();

	CEngine::get().run();

	CEngine::get().end();

	return 0;
}

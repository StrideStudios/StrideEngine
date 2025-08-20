#include "EditorRenderer.h"
#include "Engine.h"
#include "Scene.h"
#include "renderer/VulkanRenderer.h"
#include "EngineSettings.h"

#define SETTINGS_CATEGORY "Engine"
ADD_COMMAND(int32, UseFrameCap, 180, 0, 500);
ADD_TEXT(FrameRate);
ADD_TEXT(AverageFrameRate);
ADD_TEXT(GameTime);
ADD_TEXT(DeltaTime);
#undef SETTINGS_CATEGORY

int main() {

	const auto renderer = new CEditorRenderer();
	CRenderer::set(renderer);

	CEngine::get().init();

	CScene::get().init();

	CEngine::get().run();

	CScene::get().destroy();

	CEngine::get().end();

	return 0;
}
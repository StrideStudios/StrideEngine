#include "EditorRenderer.h"
#include "Engine.h"
#include "Scene.h"
#include "renderer/VulkanRenderer.h"
#include "EngineSettings.h"
#include "passes/MeshPass.h"

int main() {

	const auto renderer = new CEditorRenderer();
	CRenderer::set(renderer);

	CResourceManager passManager;

	CEngine::get().init();

	CMeshPass::enable(passManager);

	CScene::get().init();

	CEngine::get().run();

	CScene::get().destroy();

	CEngine::get().end();

	passManager.flush();

	return 0;
}
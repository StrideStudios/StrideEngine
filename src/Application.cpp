#include "EditorRenderer.h"
#include "Engine.h"
#include "Profiling.h"
#include "base/Scene.h"
#include "passes/MeshPass.h"
#include "passes/SpritePass.h"

int main() {

	CProfiler::StartupProfiler();

	CEngine::get().init();

	// Create a renderer with certain passes
	CRenderer::create<CEditorRenderer>({
		CMeshPass::make(),
		CSpritePass::make()
	});

	CScene::get().init();

	CEngine::get().run();

	// Wait for the gpu to finish instructions
	if (!CRenderer::get()->wait()) {
		errs("Engine did not stop properly!");
	}

	// Stop 'main thread'
	CThreading::getMainThread().stop();

	CScene::get().destroy();

	CResourceManager::get().flush();

	CProfiler::ShutdownProfiler();

	return 0;
}

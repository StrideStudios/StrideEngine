
// This include ensures static calls are done before main (even in libraries)
// Without it, the linker might drop symbols that need to run before main (like factory registration)
#include "Sources.h"

int main() {

	CProfiler::StartupProfiler();

	CEngine::get().init();

	// Create a renderer with certain passes
	CRenderer::create<CEditorRenderer>();

	auto cname = CStaticMeshObject::staticClass()->getClassName();
	auto name = CStaticMeshObject::staticClass()->getParent()->getClassName();
	auto name2 = CStaticMeshObject::staticClass()->getParent()->getParent()->getClassName();
	auto name3 = CStaticMeshObject::staticClass()->getParent()->getParent()->getParent()->getClassName();

	CStaticMeshObject s;
	SObject& obj = s;

	msgs("Class Name: {}, Parent Class Name: {}, Parent Parent Class Name: {}, Parent Parent Parent Class Name: {}", cname, name, name2, name3);
	msgs("Does Inherit: {}", obj.getClass()->doesInherit(CSceneObject::staticClass()));
	msgs("Equals: {}", obj.getClass() == CSceneObject::staticClass());

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


// This include ensures static calls are done before main (even in libraries)
// Without it, the linker might drop symbols that need to run before main (like factory registration)
#include "Sources.h"

int main() {

	CProfiler::StartupProfiler();

	CEngine::get().init();

	// Create a renderer with certain passes
	CRenderer::create<CEditorRenderer>();

	auto name = CStaticMeshObject::staticClass()->getClassName();
	auto name1 = CStaticMeshObject::staticClass()->getParent()->getClassName();
	auto name2 = CStaticMeshObject::staticClass()->getParent()->getParent()->getClassName();
	auto name3 = CStaticMeshObject::staticClass()->getParent()->getParent()->getParent()->getClassName();

	std::shared_ptr<CStaticMeshObject> s = std::static_pointer_cast<CStaticMeshObject>(CStaticMeshObject::staticClass()->construct());
	std::shared_ptr<SObject> obj = std::static_pointer_cast<SObject>(s);

	auto name10 = obj->getClass()->getClassName();
	auto name11 = obj->getClass()->getParent()->getClassName();
	auto name12 = obj->getClass()->getParent()->getParent()->getClassName();
	auto name13 = obj->getClass()->getParent()->getParent()->getParent()->getClassName();

	msgs("Class Name: {}, Parent Class Name: {}, Parent Parent Class Name: {}, Parent Parent Parent Class Name: {}", name, name1, name2, name3);
	msgs("NEXT: Class Name: {}, Parent Class Name: {}, Parent Parent Class Name: {}, Parent Parent Parent Class Name: {}", name10, name11, name12, name13);
	msgs("Does Inherit: {}", obj->getClass()->doesInherit(CSceneObject::staticClass()));
	msgs("Equals: {}", obj->getClass() == CSceneObject::staticClass());

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

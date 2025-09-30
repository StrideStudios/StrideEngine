
// This include ensures static calls are done before main (even in libraries)
// Without it, the linker might drop symbols that need to run before main (like factory registration)
#include "Sources.h"

int main() {

	CProfiler::StartupProfiler();

	CEngine::get().init();

	// Create a renderer with certain passes
	CRenderer::create<CEditorRenderer>();

	auto name = CStaticMeshObject::staticClass()->getName();
	auto name1 = CStaticMeshObject::staticClass()->getParent()->getName();
	auto name2 = CStaticMeshObject::staticClass()->getParent()->getParent()->getName();
	auto name3 = CStaticMeshObject::staticClass()->getParent()->getParent()->getParent()->getName();
	auto name4 = CStaticMeshObject::staticClass()->getParent()->getParent()->getParent()->getParent()->getName();
	auto name5 = CStaticMeshObject::staticClass()->getParent()->getParent()->getParent()->getParent()->getParent()->getName();

	std::shared_ptr<CStaticMeshObject> s = std::static_pointer_cast<CStaticMeshObject>(CStaticMeshObject::staticClass()->construct());
	std::shared_ptr<SObject> obj = std::static_pointer_cast<SObject>(s);

	//auto sc = s->getClass();
	auto sc = CStaticMeshObject::staticClass();


	if (auto c = std::dynamic_pointer_cast<IRenderableClass>(sc)) {
		msgs("is renderable");
	}

	auto name10 = obj->getClass()->getName();
	auto name11 = obj->getClass()->getParent()->getName();
	auto name12 = obj->getClass()->getParent()->getParent()->getName();
	auto name13 = obj->getClass()->getParent()->getParent()->getParent()->getName();
	auto name14 = obj->getClass()->getParent()->getParent()->getParent()->getParent()->getName();
	auto name15 = obj->getClass()->getParent()->getParent()->getParent()->getParent()->getParent()->getName();

	msgs("Class Name: {}, Name1: {}, Name2: {}, Name3: {}, Name4: {}, Name5: {}", name, name1, name2, name3, name4, name5);
	msgs("NEXT: Class Name: {}, Name1: {}, Name2: {}, Name3: {}, Name4: {}, Name5: {}", name10, name11, name12, name13, name14, name15);
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

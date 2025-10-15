
// This include ensures static calls are done before main (even in libraries)
// Without it, the linker might drop symbols that need to run before main (like factory registration)
#include "Sources.h"

class CTestClass : public CStaticMeshObject {

	REGISTER_CLASS(CTestClass, CStaticMeshObject)

};

class CTestClassObjectRenderer : public CWorldObjectRenderer<CTestClass, CMeshPass> {

	REGISTER_RENDERER(CTestClassObjectRenderer, CTestClass, CWorldObjectRenderer<CTestClass, CMeshPass>)

public:

	virtual void begin() override {
		m_LastIndexBuffer = VK_NULL_HANDLE;
	}

	virtual void render(CMeshPass* inPass, VkCommandBuffer cmd, CTestClass* inObject, uint32& outDrawCalls, uint64& outVertices) override {}

private:

	VkBuffer m_LastIndexBuffer = VK_NULL_HANDLE;

};

int main() {

	CProfiler::StartupProfiler();

	// Create a renderer with certain passes
	CRenderer::create<CEditorRenderer>();

	CEngine::get().run();

	// Wait for the gpu to finish instructions
	if (!CRenderer::get()->wait()) {
		errs("Engine did not stop properly!");
	}

	// Stop 'main thread'
	CThreading::getMainThread().stop();

	CResourceManager::get().flush();

	CProfiler::ShutdownProfiler();

	return 0;
}

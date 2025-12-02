
// This include ensures static calls are done before main (even in libraries)
// Without it, the linker might drop symbols that need to run before main (like factory registration)
#include "Sources.h"
#include "basic/containers/Array.h"

struct SHello : SObject {
	REGISTER_STRUCT(SHello, SObject)

	SHello() = default;

	SHello(const std::string& name): name(name) {}

	void print() {
		msgs("Name: {}", name);
}

	std::string name = "None";
};

int main() {

	// Create a renderer with certain passes
	/*CRenderer::create<CEditorRenderer>();

	CEngine::get().run();

	// Wait for the gpu to finish instructions
	if (!CRenderer::get()->wait()) {
		errs("Engine did not stop properly!");
	}

	// Stop 'main thread'
	CThreading::getMainThread().stop();

	CResourceManager::get().flush();*/

	{
		TArray<TUnique<SHello>, 10> v;
		v.fill(SHello{"Arrayg"});
		v.set(0, SHello{"Arrayg1"});
		v.forEachReverse([](TUnique<SHello>& object) {
			object->print();
		});
	}

	{
		TVector<TUnique<SHello>> v;
		v.add(SHello{"Unique One"});
		v.add(SHello{"Unique Two"});
		v.forEachReverse([](TUnique<SHello>& object) {
			object->print();
		});
	}

	{
		TVector<TShared<SHello>> v;
		v.add(SHello{"Shared One"});
		v.add(SHello{"Shared Two"});
		v.forEachReverse([](TShared<SHello>& object) {
			object->print();
		});
	}

	{
		TVector<SHello> v;
		v.add(SHello{
			"Hello One"
		});
		v.forEachReverse([](SHello& object) {
			object.print();
		});
	}

	return 0;
}

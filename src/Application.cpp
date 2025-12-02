
// This include ensures static calls are done before main (even in libraries)
// Without it, the linker might drop symbols that need to run before main (like factory registration)
#include "Sources.h"
#include "basic/containers/Array.h"
#include "basic/containers/List.h"

struct SHello : SObject {
	REGISTER_STRUCT(SHello, SObject)

	SHello() = default;

	SHello(const std::string& name): name(name) {}

	void print() {
		std::cout << name;
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

	constexpr size_t amt = 100000;

	{
		auto previousTime = std::chrono::high_resolution_clock::now();
		TArray<TUnique<SHello>, amt> v;
		v.fill(SHello{"Unique Array"});
		v.forEachReverse([](TUnique<SHello>& object) {
			//object->print();
		});
		std::cout << std::endl;
		msgs("Unique Array Time Taken: {}", std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - previousTime).count());
	}

	{
		auto previousTime = std::chrono::high_resolution_clock::now();
		TList<TUnique<SHello>> v;
		for (int i = 0; i < amt; ++i)
			v.add(SHello{"Unique List"});
		v.forEachReverse([](TUnique<SHello>& object) {
			//object->print();
		});
		std::cout << std::endl;
		msgs("Unique List Time Taken: {}", std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - previousTime).count());
	}

	{
		auto previousTime = std::chrono::high_resolution_clock::now();
		TVector<TUnique<SHello>> v;
		for (int i = 0; i < amt; ++i)
			v.add(SHello{"Unique Vector"});
		v.forEachReverse([](TUnique<SHello>& object) {
			//object->print();
		});
		std::cout << std::endl;
		msgs("Unique Vector Time Taken: {}", std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - previousTime).count());
	}

	{
		auto previousTime = std::chrono::high_resolution_clock::now();
		TVector<TShared<SHello>> v;
		for (int i = 0; i < amt; ++i)
			v.add(SHello{"Shared Vector"});
		v.forEachReverse([](TShared<SHello>& object) {
			//object->print();
		});
		std::cout << std::endl;
		msgs("Shared Vector Time Taken: {}", std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - previousTime).count());
	}

	/*{
		auto previousTime = std::chrono::high_resolution_clock::now();
		TVector<SHello> v;
		for (int i = 0; i < amt; ++i)
			v.add(SHello{"NA Vector"});
		v.forEachReverse([](SHello& object) {
			//object.print();
		});
		std::cout << std::endl;
		msgs("NA Vector Time Taken: {}", std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - previousTime).count());
	}*/

	return 0;
}


// This include ensures static calls are done before main (even in libraries)
// Without it, the linker might drop symbols that need to run before main (like factory registration)
#include "Sources.h"

struct SHello : SObject {
	REGISTER_STRUCT(SHello, SObject)

	SHello() = default;

	SHello(const size_t id, const std::string& name):
	id(id),
	name(name) {}

	SHello(const std::string& name):
	SHello(0, name) {}

	void print() {
		msgs("ID: {}, Name: {}", id, name);
	}

	size_t id = 0;
	std::string name = "None";

	friend bool operator<(const SHello& fst, const SHello& snd) {
		return fst.id < snd.id;
	}

	friend bool operator==(const SHello& fst, const SHello& snd) {
		return fst.id == snd.id;
	}

	friend size_t getHash(const SHello& obj) {
		return getHash(obj.name) + obj.id;
	}
};

int main() {

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

	return 0;
}

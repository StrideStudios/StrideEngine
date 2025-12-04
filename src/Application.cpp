
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
	/*CRenderer::create<CEditorRenderer>();

	CEngine::get().run();

	// Wait for the gpu to finish instructions
	if (!CRenderer::get()->wait()) {
		errs("Engine did not stop properly!");
	}

	// Stop 'main thread'
	CThreading::getMainThread().stop();

	CResourceManager::get().flush();*/

	constexpr size_t amt = 5;

	{
		auto previousTime = std::chrono::high_resolution_clock::now();
		TArray<TUnique<SHello>, amt> v;
		v.fill(SHello{"Unique Array"});
		v.forEachReverse([](size_t index, TUnique<SHello>& object) {
			//object->print();
		});
		std::cout << std::endl;
		msgs("Unique Array Time Taken: {}", std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - previousTime).count());
	}

	{
		auto previousTime = std::chrono::high_resolution_clock::now();
		TList<TUnique<SHello>> v;
		for (size_t i = 0; i < amt; ++i)
			v.add(SHello{i, "Unique List"});
		v.forEachReverse([](size_t index, TUnique<SHello>& object) {
			//object->print();
		});
		std::cout << std::endl;
		msgs("Unique List Time Taken: {}", std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - previousTime).count());
	}

	{
		auto previousTime = std::chrono::high_resolution_clock::now();
		TDeque<TUnique<SHello>> v;
		for (size_t i = 0; i < amt; ++i)
			v.add(SHello{i, "Unique Deque"});
		v.forEachReverse([](size_t index, TUnique<SHello>& object) {
			//object->print();
		});
		std::cout << std::endl;
		msgs("Unique Deque Time Taken: {}", std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - previousTime).count());
	}

	{
		auto previousTime = std::chrono::high_resolution_clock::now();
		TForwardList<TUnique<SHello>> v;
		for (size_t i = 0; i < amt; ++i)
			v.add(SHello{i, fmts("Unique Forward List {}", i)});

		v.get(0)->print();
		v.remove(0);
		msgs("");

		v.forEach([](size_t index, TUnique<SHello>& object) {
			//object->print();
		});
		std::cout << std::endl;
		msgs("Unique Forward List Time Taken: {}", std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - previousTime).count());
	}

	{
		auto previousTime = std::chrono::high_resolution_clock::now();
		TPrioritySet<TUnique<SHello>> v;
		for (size_t i = 0; i < amt; ++i)
			v.add(SHello{i, fmts("Unique Priority Set {}", i)});

		v.get(0)->print();
		v.remove(0);
		msgs("");

		v.forEach([](size_t index, TUnique<SHello>& object) {
			std::cout << "Index: " << index << " ";
			object->print();
		});
		std::cout << std::endl;
		msgs("Unique Priority Set Time Taken: {}", std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - previousTime).count());
	}

	{
		auto previousTime = std::chrono::high_resolution_clock::now();
		TSet<TShared<SHello>> v;

		TShared fst = SHello{"Shared Set fst"};

		v.add(fst);

		for (size_t i = 0; i < amt; ++i)
			v.add(SHello{i, fmts("Shared Set {}", i)});

		v.get(0)->print();
		v.remove(0);
		msgs("");

		v.forEach([](size_t index, TShared<SHello>& object) {
			object->print();
		});
		std::cout << std::endl;
		msgs("Shared Set Time Taken: {}", std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - previousTime).count());
	}

	{
		auto previousTime = std::chrono::high_resolution_clock::now();
		TQueue<TShared<SHello>> v;

		v.add(SHello{3, fmts("Shared Queue {}", 3)});
		v.add(SHello{4, fmts("Shared Queue {}", 4)});
		v.add(SHello{1, fmts("Shared Queue {}", 1)});
		v.add(SHello{2, fmts("Shared Queue {}", 2)});
		v.add(SHello{0, fmts("Shared Queue {}", 0)});

		v.forEachReverse([](size_t index, TShared<SHello>& object) {
			std::cout << "Index: " << index << " ";
			object->print();
		});
		std::cout << std::endl;
		msgs("Shared Queue Time Taken: {}", std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - previousTime).count());
	}

	{
		auto previousTime = std::chrono::high_resolution_clock::now();
		TPriorityQueue<TShared<SHello>> v;

		v.add(SHello{3, fmts("Shared Priority Queue {}", 3)});
		v.add(SHello{4, fmts("Shared Priority Queue {}", 4)});
		v.add(SHello{1, fmts("Shared Priority Queue {}", 1)});
		v.add(SHello{2, fmts("Shared Priority Queue {}", 2)});
		v.add(SHello{0, fmts("Shared Priority Queue {}", 0)});

		v.forEach([](size_t index, TShared<SHello>& object) {
			object->print();
		});
		std::cout << std::endl;
		msgs("Shared Priority Queue Time Taken: {}", std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - previousTime).count());
	}

	{
		auto previousTime = std::chrono::high_resolution_clock::now();
		TStack<TShared<SHello>> v;

		v.add(SHello{3, fmts("Shared Stack {}", 3)});
		v.add(SHello{4, fmts("Shared Stack {}", 4)});
		v.add(SHello{1, fmts("Shared Stack {}", 1)});
		v.add(SHello{2, fmts("Shared Stack {}", 2)});
		v.add(SHello{0, fmts("Shared Stack {}", 0)});

		v.forEach([](size_t index, TShared<SHello>& object) {
			object->print();
		});
		std::cout << std::endl;
		msgs("Shared Stack Time Taken: {}", std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - previousTime).count());
	}

	{
		auto previousTime = std::chrono::high_resolution_clock::now();
		TPriorityMap<char, TUnique<SHello>> v;

		v.set('C', SHello{3, fmts("Unique Priority Map {}", 3)});
		v.set('G', SHello{4, fmts("Unique Priority Map {}", 4)});
		v.set('F', SHello{1, fmts("Unique Priority Map {}", 1)});
		v.set('A', SHello{2, fmts("Unique Priority Map {}", 2)});
		v.set('P', SHello{0, fmts("Unique Priority Map {}", 0)});

		v.forEach([](const char& index, TUnique<SHello>& object) {
			std::cout << "Index: " << index << " ";
			object->print();
		});
		std::cout << std::endl;
		msgs("Unique Map Time Taken: {}", std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - previousTime).count());
	}

	{
		auto previousTime = std::chrono::high_resolution_clock::now();
		TMap<char, TUnique<SHello>> v;

		v.set('C', SHello{3, fmts("Unique Map {}", 3)});
		v.set('G', SHello{4, fmts("Unique Map {}", 4)});
		v.set('F', SHello{1, fmts("Unique Map {}", 1)});
		v.set('A', SHello{2, fmts("Unique Map {}", 2)});
		v.set('P', SHello{0, fmts("Unique Map {}", 0)});

		v.forEach([](const char& index, TUnique<SHello>& object) {
			std::cout << "Index: " << index << " ";
			object->print();
		});
		std::cout << std::endl;
		msgs("Unique Unordered Map Time Taken: {}", std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - previousTime).count());
	}

	{
		auto previousTime = std::chrono::high_resolution_clock::now();
		TVector<TUnique<SHello>> v;

		TUnique hello = SHello{0, "Unique Vector"};

		for (size_t i = 0; i < amt; ++i)
			v.add(SHello{i, "Unique Vector"});
		v.forEachReverse([](size_t index, TUnique<SHello>& object) {
			//object->print();
		});
		std::cout << std::endl;
		msgs("Unique Vector Time Taken: {}", std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - previousTime).count());
	}

	{
		auto previousTime = std::chrono::high_resolution_clock::now();
		TVector<TShared<SHello>> v;
		for (size_t i = 0; i < amt; ++i)
			v.add(SHello{i, "Shared Vector"});
		v.forEachReverse([](size_t index, TShared<SHello>& object) {
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

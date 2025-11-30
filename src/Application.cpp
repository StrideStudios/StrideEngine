
// This include ensures static calls are done before main (even in libraries)
// Without it, the linker might drop symbols that need to run before main (like factory registration)
#include "Sources.h"

template <typename TType>
struct TVector : TContainer<TType> {

	virtual const size_t getSize() const override {
		return m_Container.size();
	}

	virtual void reserve(size_t index) override {
		m_Container.reserve(index);
	}

	virtual void resize(size_t index) override {
		m_Container.resize(index);
	}

	virtual TNode<TType>& get(size_t index) override {
		return m_Container[index];
	}

	virtual const TNode<TType>& get(size_t index) const override {
		return m_Container[index];
	}

	virtual TNode<TType>& addDefaulted() override {
		m_Container.emplace_back();
		return get(getSize() - 1);
	}

	virtual size_t add(TNode<TType>&& obj) override {
		m_Container.emplace_back(std::move(obj));
		return getSize() - 1;
	}

	virtual void set(const size_t index, TNode<TType>&& obj) override {
		remove(index);
		m_Container.insert(m_Container.begin() + index, std::move(obj));
	}

	virtual TNode<TType>& remove(const size_t index) override {
		auto& obj = get(index);
		m_Container.erase(m_Container.begin() + index);
		return obj;
	}

	std::vector<TNode<TType>> m_Container;
};

// Instead of using raw pointers, we use unique_ptr internally
// This has the consequence of every object having the same lifetime as the container

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
		TVector<std::unique_ptr<SHello>> v;
		v.add(SHello{"Unique One"});
		v.add(SHello{"Unique Two"});
		v.forEach([](SHello& object) {
			msgs("Name: {}", object.name);
		});
	}

	{
		TVector<std::shared_ptr<SHello>> v;
		v.add(SHello{"Shared One"});
		v.add(SHello{"Shared Two"});
		v.forEach([](SHello& object) {
			msgs("Name: {}", object.name);
		});
	}

	{
		TVector<SHello> v;
		v.add(SHello{
			"Hello One"
		});
		v.forEach([](SHello& object) {
			msgs("Name: {}", object.name);
		});
	}

	return 0;
}

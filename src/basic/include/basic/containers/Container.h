#pragma once

#include "basic/core/Common.h"

// A basic container of any amount of objects
// A size of 0 implies a dynamic array
template <typename TType, size_t TSize = 0>
struct TContainer {

	using Storage = TType;

	virtual ~TContainer() = default;

	virtual const size_t getSize() const = 0;
	constexpr size_t getCapacity() {
		return TSize;
	}

	virtual void reserve(size_t index) = 0;
	virtual void resize(size_t index) = 0;

	virtual Storage& get(size_t index) = 0;
	virtual const Storage& get(size_t index) const = 0;

	virtual Storage& operator[](size_t index) { return get(index); }
	virtual const Storage& operator[](size_t index) const { return get(index); }

	virtual Storage& addDefaulted() = 0;

	virtual size_t add(Storage&& obj) = 0;
	virtual void set(size_t index, Storage&& obj) = 0;

	virtual Storage& remove(size_t index) = 0;

	// TODO: data, begin, and end implies contiguous memory, unlike a list, set, map, queue, or stack
	// Perhaps wrappers around non-contiguous memory could use a Node<> template (with a pointer to the next address, of course)
	Storage* data() { return begin(); }
	const Storage* data() const { return begin(); }

	Storage* begin() { return &get(0); }
	const Storage* begin() const { return &get(0); }

	Storage* end() { return &get(getSize() - 1) + 1; }
	const Storage* end() const { return &get(getSize() - 1) + 1; }

	Storage* rbegin() { return &get(getSize() - 1); }
	const Storage* rbegin() const { return &get(getSize() - 1); }

	Storage* rend() { return &get(0) - 1; }
	const Storage* rend() const { return &get(0) - 1; }

	template <typename TFunc>
	void forEach(TFunc&& func) {
		for (auto itr = begin(); itr != end(); ++itr) {
			func(*itr);
		}
	}

	template <typename TFunc>
	void forEachReverse(TFunc&& func) {
		for (auto itr = rbegin(); itr != rend(); --itr) {
			func(*itr);
		}
	}
};
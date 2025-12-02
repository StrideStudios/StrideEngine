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
	Storage* data() { return &get(0); }
	const Storage* data() const { return &get(0); }

	virtual void forEach(const std::function<void(TType&)>& func) = 0;

	virtual void forEachReverse(const std::function<void(TType&)>& func) = 0;
};
#pragma once

#include "basic/core/Common.h"

template <bool _Test>
struct valid_type_if {
	constexpr static size_t size = 0;
};

template <>
struct valid_type_if<true> {
	constexpr static size_t size = 1;
};

template <bool _Test>
using valid_type_if_v = valid_type_if<_Test>::size;


template <class TType>
struct is_copyable : std::is_nothrow_copy_assignable<TType>, std::is_copy_constructible<TType> {};

template <class TType>
constexpr bool is_copyable_v = is_copyable<TType>::value;

template <class TType>
struct is_moveable : std::is_nothrow_move_assignable<TType>, std::is_move_constructible<TType> {};

template <class TType>
constexpr bool is_moveable_v = is_moveable<TType>::value;

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

	virtual size_t add(const Storage& obj) = 0;
	virtual size_t add(Storage&& obj) = 0;
	virtual void set(size_t index, const Storage& obj) = 0;
	virtual void set(size_t index, Storage&& obj) = 0;

	virtual Storage& remove(size_t index) = 0;

	// TODO: data, begin, and end implies contiguous memory, unlike a list, set, map, queue, or stack
	// Perhaps wrappers around non-contiguous memory could use a Node<> template (with a pointer to the next address, of course)
	Storage* data() { return &get(0); }
	const Storage* data() const { return &get(0); }

	virtual void forEach(const std::function<void(TType&)>& func) = 0;

	virtual void forEachReverse(const std::function<void(TType&)>& func) = 0;

private:

	/*
	 * Some Containers need different functions, these are not guaranteed, but can be used
	 */

	virtual size_t push(const Storage& obj) {
		if constexpr (is_copyable_v<TType>) {
			return add(obj);
		} else {
			errs("Type is not copyable!");
		}
	}

	virtual size_t push(Storage&& obj) {
		if constexpr (is_moveable_v<TType>) {
			return add(std::move(obj));
		} else {
			errs("Type is not moveable!");
		}
	}

	virtual Storage& pop() {
		auto& val = get(0);
		remove(0);
		return val;
	}

	virtual Storage& top() { return get(0); }

	virtual const Storage& top() const { return get(0); }

	virtual void replace(const Storage&, Storage&&) {}

	virtual void remove(const Storage&) {}
};
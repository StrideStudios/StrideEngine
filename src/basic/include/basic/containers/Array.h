#pragma once

#include <array>
#include "basic/containers/Container.h"

template <typename TType, size_t TSize>
struct TArray : TContainer<TType, TSize> {

	virtual const size_t getSize() const override {
		return m_Container.size();
	}

	template <typename... TArgs>
	void fill(TArgs&&... args) {
		for (auto& obj : m_Container) {
			obj = TNode<TType>(args...);
		}
	}

	virtual TNode<TType>& get(size_t index) override {
		return m_Container[index];
	}

	virtual const TNode<TType>& get(size_t index) const override {
		return m_Container[index];
	}

	virtual void set(const size_t index, TNode<TType>&& obj) override {
		m_Container[index] = std::move(obj);
	}

private:

	virtual void reserve(size_t index) override {
		errs("TArray cannot be allocated after creation!");
	}

	virtual void resize(size_t index) override {
		errs("TArray cannot be allocated after creation!");
	}

	virtual TNode<TType>& addDefaulted() override {
		errs("TArray cannot be allocated after creation!");
	}

	virtual size_t add(TNode<TType>&& obj) override {
		errs("TArray cannot be allocated after creation!");
	}

	virtual TNode<TType>& remove(const size_t index) override {
		errs("TArray cannot be allocated after creation!");
	}

	std::array<TNode<TType>, TSize> m_Container;
};
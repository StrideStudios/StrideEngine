#pragma once

#include <vector>
#include "basic/containers/Container.h"

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
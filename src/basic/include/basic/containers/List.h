#pragma once

#include <list>
#include "basic/core/Object.h"
#include "basic/containers/Container.h"

template <typename TType>
struct TList : TContainer<TType> {

	virtual const size_t getSize() const override {
		return m_Container.size();
	}

	virtual void resize(size_t index) override {
		m_Container.resize(index);
	}

	virtual TType& get(size_t index) override {
		auto it = m_Container.begin();
		std::advance(it, index);
		return *it;
	}

	virtual const TType& get(size_t index) const override {
		auto it = m_Container.begin();
		std::advance(it, index);
		return *it;
	}

	virtual TType& addDefaulted() override {
		m_Container.emplace_back();
		return get(getSize() - 1);
	}

	virtual size_t add(TType&& obj) override {
		m_Container.emplace_back(std::move(obj));
		return getSize() - 1;
	}

	virtual void set(const size_t index, TType&& obj) override {
		remove(index);
		auto it = m_Container.begin();
		std::advance(it, index);
		m_Container.insert(it, std::move(obj));
	}

	virtual TType& remove(const size_t index) override {
		auto& obj = get(index);
		auto it = m_Container.begin();
		std::advance(it, index);
		m_Container.erase(it);
		return obj;
	}

	virtual void forEach(const std::function<void(TType&)>& func) override {
		for (auto itr = m_Container.begin(); itr != m_Container.end(); std::advance(itr, 1)) {
			func(*itr);
		}
	}

	virtual void forEachReverse(const std::function<void(TType&)>& func) override {
		for (auto itr = m_Container.rbegin(); itr != m_Container.rend(); std::advance(itr, 1)) {
			func(*itr);
		}
	}

private:

	virtual void reserve(size_t index) override {
		errs("TList does not have contiguous memory!");
	}

	std::list<TType> m_Container;
};
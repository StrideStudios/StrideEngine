#pragma once

#include <list>
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
		auto itr = m_Container.begin();
		std::advance(itr, index);
		return *itr;
	}

	virtual const TType& get(size_t index) const override {
		auto itr = m_Container.begin();
		std::advance(itr, index);
		return *itr;
	}

	virtual TType& addDefaulted() override {
		m_Container.emplace_back();
		return get(getSize() - 1);
	}

	virtual size_t add(const TType& obj) override {
		if constexpr (is_copyable_v<TType>) {
			m_Container.emplace_back(obj);
			return getSize() - 1;
		} else {
			errs("Type is not copyable!");
		}
	}

	virtual size_t add(TType&& obj) override {
		if constexpr (is_moveable_v<TType>) {
			m_Container.emplace_back(std::move(obj));
			return getSize() - 1;
		} else {
			errs("Type is not moveable!");
		}
	}

	virtual void set(const size_t index, const TType& obj) override {
		if constexpr (is_copyable_v<TType>) {
			remove(index);
			auto itr = m_Container.begin();
			std::advance(itr, index);
			m_Container.insert(itr, obj);
		} else {
			errs("Type is not copyable!");
		}
	}

	virtual void set(const size_t index, TType&& obj) override {
		if constexpr (is_moveable_v<TType>) {
			remove(index);
			auto itr = m_Container.begin();
			std::advance(itr, index);
			m_Container.insert(itr, std::move(obj));
		} else {
			errs("Type is not moveable!");
		}
	}

	virtual TType& remove(const size_t index) override {
		auto& obj = get(index);
		auto itr = m_Container.begin();
		std::advance(itr, index);
		m_Container.erase(itr);
		return obj;
	}

	virtual void forEach(const std::function<void(size_t, TType&)>& func) override {
		size_t i = 0;
		for (auto itr = m_Container.begin(); itr != m_Container.end(); std::advance(itr, 1), ++i) {
			func(i, *itr);
		}
	}

	virtual void forEachReverse(const std::function<void(size_t, TType&)>& func) override {
		size_t i = getSize() - 1;
		for (auto itr = m_Container.rbegin(); itr != m_Container.rend(); std::advance(itr, 1), --i) {
			func(i, *itr);
		}
	}

private:

	virtual void reserve(size_t index) override {
		errs("TList does not have contiguous memory!");
	}

	std::list<TType> m_Container;
};
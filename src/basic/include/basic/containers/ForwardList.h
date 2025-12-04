#pragma once

#include <forward_list>
#include "basic/containers/Container.h"

//#include "basic/core/Object.h"

//typedef SObject TType;

template <typename TType>
struct TForwardList : TContainer<TType> {

	virtual const size_t getSize() const override {
		return m_Size;
	}

	virtual void resize(size_t index) override {
		m_Container.resize(index);
		m_Size = index;
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
		m_Container.emplace_front();
		m_Size++;
		return get(getSize() - 1);
	}

	virtual size_t add(const TType& obj) override {
		if constexpr (is_copyable_v<TType>) {
			m_Container.emplace_front(obj);
			m_Size++;
			return getSize() - 1;
		} else {
			errs("Type is not copyable!");
		}
	}

	virtual size_t add(TType&& obj) override {
		if constexpr (is_moveable_v<TType>) {
			m_Container.emplace_front(std::move(obj));
			m_Size++;
			return getSize() - 1;
		} else {
			errs("Type is not moveable!");
		}
	}

	virtual void set(const size_t index, const TType& obj) override {
		if constexpr (is_copyable_v<TType>) {
			remove(index);
			auto itr = m_Container.before_begin();
			std::advance(itr, index);
			m_Container.insert_after(itr, obj);
			m_Size++;
		} else {
			errs("Type is not copyable!");
		}
	}

	virtual void set(const size_t index, TType&& obj) override {
		if constexpr (is_moveable_v<TType>) {
			remove(index);
			auto itr = m_Container.before_begin();
			std::advance(itr, index);
			m_Container.insert_after(itr, std::move(obj));
			m_Size++;
		} else {
			errs("Type is not moveable!");
		}
	}

	virtual TType& remove(const size_t index) override {
		auto& obj = get(index);
		auto itr = m_Container.before_begin();
		std::advance(itr, index);
		m_Container.erase_after(itr);
		m_Size--;
		return obj;
	}

	virtual void forEach(const std::function<void(TType&)>& func) override {
		for (auto itr = m_Container.begin(); itr != m_Container.end(); std::advance(itr, 1)) {
			func(*itr);
		}
	}

private:

	virtual void reserve(size_t index) override {
		errs("TForwardList does not have contiguous memory!");
	}

	virtual void forEachReverse(const std::function<void(TType&)>& func) override {
		errs("TForwardList cannot be iterated in reverse!");
	}

	std::forward_list<TType> m_Container;
	size_t m_Size = 0;
};
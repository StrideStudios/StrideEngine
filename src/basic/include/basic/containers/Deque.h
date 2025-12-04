#pragma once

#include <deque>
#include "basic/containers/Container.h"

template <typename TType>
struct TDeque : TContainer<TType> {

	virtual const size_t getSize() const override {
		return m_Container.size();
	}

	virtual void resize(size_t index) override {
		m_Container.resize(index);
	}

	virtual TType& get(size_t index) override {
		return m_Container[index];
	}

	virtual const TType& get(size_t index) const override {
		return m_Container[index];
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
			m_Container.insert(m_Container.begin() + index, obj);
		} else {
			errs("Type is not copyable!");
		}
	}

	virtual void set(const size_t index, TType&& obj) override {
		if constexpr (is_moveable_v<TType>) {
			remove(index);
			m_Container.insert(m_Container.begin() + index, std::move(obj));
		} else {
			errs("Type is not moveable!");
		}
	}

	virtual TType& remove(const size_t index) override {
		auto& obj = get(index);
		m_Container.erase(m_Container.begin() + index);
		return obj;
	}

	virtual void forEach(const std::function<void(TType&)>& func) override {
		for (auto itr = m_Container.begin(); itr != m_Container.end(); ++itr) {
			func(*itr);
		}
	}

	virtual void forEachReverse(const std::function<void(TType&)>& func) override {
		for (auto itr = m_Container.rbegin(); itr != m_Container.rend(); ++itr) {
			func(*itr);
		}
	}

private:

	virtual void reserve(size_t index) override {
		errs("TDeque does not have contiguous memory!");
	}

	std::deque<TType> m_Container;
};
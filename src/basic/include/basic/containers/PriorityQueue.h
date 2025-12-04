#pragma once

#include <queue>
#include "basic/containers/Container.h"

template <typename TType>
struct TPriorityQueue : TContainer<TType> {

	virtual const size_t getSize() const override {
		return m_Container.size();
	}

	virtual TType& pop() override {
		auto& val = top();
		m_Container.pop();
		return val;
	}

	virtual TType& top() override {
		return const_cast<TType&>(m_Container.top());
	}

	virtual const TType& top() const override {
		return m_Container.top();
	}

	virtual TType& get(size_t index) override {
		if (index == 0) return top();
		errs("TPriorityQueue does not support random access!");
	}

	virtual const TType& get(size_t index) const override {
		if (index == 0) return top();
		errs("TPriorityQueue does not support random access!");
	}

	virtual TType& addDefaulted() override {
		m_Container.emplace();
		return get(getSize() - 1);
	}

	virtual size_t add(const TType& obj) override {
		//static_assert(is_copyable_v<TType>, "Type is not copyable!");
		if constexpr (is_copyable_v<TType>) {
			m_Container.emplace(obj);
			return getSize() - 1;
		} else {
			errs("Type is not copyable");
		}
	}

	virtual size_t add(TType&& obj) override {
		//static_assert(is_moveable_v<TType>, "Type is not moveable!");
		if constexpr (is_moveable_v<TType>) {
			m_Container.emplace(std::move(obj));
			return getSize() - 1;
		} else {
			errs("Type is not moveable");
		}
	}

	virtual void forEach(const std::function<void(TType&)>& func) override {
		for (auto itr = m_Container.getContainer().begin(); itr != m_Container.getContainer().end(); ++itr) {
			func(*itr);
		}
	}

	virtual void forEachReverse(const std::function<void(TType&)>& func) override {
		for (auto itr = m_Container.getContainer().rbegin(); itr != m_Container.getContainer().rend(); ++itr) {
			func(*itr);
		}
	}

private:

	virtual void reserve(size_t index) override {
		errs("TPriorityQueue does not have contiguous memory!");
	}

	virtual void resize(size_t index) override {
		errs("TPriorityQueue does not have contiguous memory!");
	}

	virtual void set(const size_t index, const TType& obj) override {
		errs("TPriorityQueue does not support random access!");
	}

	virtual void set(const size_t index, TType&& obj) override {
		errs("TPriorityQueue does not support random access!");
	}

	virtual TType& remove(const size_t index) override {
		errs("TPriorityQueue does not support random access!");
	}

	struct IterablePriorityQueue : std::priority_queue<TType> {
		std::vector<TType>& getContainer() {
			return std::priority_queue<TType>::c;
		}
	} m_Container;
};
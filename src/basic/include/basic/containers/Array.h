#pragma once

#include <array>
#include "basic/containers/Container.h"

template <typename TType, size_t TSize>
struct TArray : TContainer<TType, size_t, TSize> {
	virtual const size_t getSize() const override {
		return m_Container.size();
	}

	template <typename... TArgs>
	void fill(TArgs&&... args) {
		for (auto& obj : m_Container) {
			obj = TType(args...);
		}
	}

	virtual TType& get(size_t index) override {
		return m_Container[index];
	}

	virtual const TType& get(size_t index) const override {
		return m_Container[index];
	}

	virtual void set(const size_t index, const TType& obj) override {
		if constexpr (is_copyable_v<TType>) {
			m_Container[index] = obj;
		} else {
			errs("Type is not copyable!");
		}
	}

	virtual void set(const size_t index, TType&& obj) override {
		if constexpr (is_moveable_v<TType>) {
			m_Container[index] = std::move(obj);
		} else {
			errs("Type is not moveable!");
		}
	}

	virtual void forEach(const std::function<void(size_t, TType&)>& func) override {
		size_t i = 0;
		for (auto itr = m_Container.begin(); itr != m_Container.end(); ++itr, ++i) {
			func(i, *itr);
		}
	}

	virtual void forEachReverse(const std::function<void(size_t, TType&)>& func) override {
		size_t i = getSize() - 1;
		for (auto itr = m_Container.rbegin(); itr != m_Container.rend(); ++itr, --i) {
			func(i, *itr);
		}
	}

private:

	virtual void reserve(size_t index) override {
		errs("TArray cannot be allocated after creation!");
	}

	virtual void resize(size_t index) override {
		errs("TArray cannot be allocated after creation!");
	}

	virtual TType& addDefaulted() override {
		errs("TArray cannot be allocated after creation!");
	}

	virtual size_t add(const TType& obj) override {
		errs("TArray cannot be allocated after creation!");
	}

	virtual size_t add(TType&& obj) override {
		errs("TArray cannot be allocated after creation!");
	}

	virtual TType& remove(const size_t index) override {
		errs("TArray cannot be allocated after creation!");
	}

	std::array<TType, TSize> m_Container;
};
#pragma once

#include <unordered_set>
#include "basic/containers/Container.h"

template <typename TType>
struct TSet : TContainer<TType> {

	virtual const size_t getSize() const override {
		return m_Container.size();
	}

	virtual TType& get(size_t index) override {
		auto itr = m_Container.begin();
		std::advance(itr, index);
		return const_cast<TType&>(*itr);
	}

	virtual const TType& get(size_t index) const override {
		auto itr = m_Container.begin();
		std::advance(itr, index);
		return *itr;
	}

	virtual TType& addDefaulted() override {
		m_Container.emplace();
		return get(getSize() - 1);
	}

	virtual size_t add(const TType& obj) override {
		if constexpr (is_copyable_v<TType>) {
			m_Container.emplace(obj);
			return getSize() - 1;
		} else {
			errs("Type is not copyable!");
		}
	}

	virtual size_t add(TType&& obj) override {
		if constexpr (is_moveable_v<TType>) {
			m_Container.emplace(std::move(obj));
			return getSize() - 1;
		} else {
			errs("Type is not moveable!");
		}
	}

	virtual void set(const size_t index, const TType& obj) override {
		if constexpr (is_copyable_v<TType>) {
			remove(index);
			auto it = m_Container.begin();
			std::advance(it, index);
			m_Container.insert(it, obj);
		} else {
			errs("Type is not copyable!");
		}
	}

	virtual void set(const size_t index, TType&& obj) override {
		if constexpr (is_moveable_v<TType>) {
			remove(index);
			auto it = m_Container.begin();
			std::advance(it, index);
			m_Container.insert(it, std::move(obj));
		} else {
			errs("Type is not moveable!");
		}
	}

	virtual void replace(const TType& tgt, TType&& obj) override {
		// Since this container is unordered, replacing doesn't need to set at the same index
		remove(tgt);
		m_Container.insert(std::move(obj));
	}

	virtual TType& remove(const size_t index) override {
		auto& obj = get(index);
		auto itr = m_Container.begin();
		std::advance(itr, index);
		m_Container.erase(itr);
		return obj;
	}

	virtual void remove(const TType& obj) override {
		m_Container.erase(obj);
	}

	virtual void forEach(const std::function<void(size_t, TType&)>& func) override {
		size_t i = 0;
		for (auto itr = m_Container.begin(); itr != m_Container.end(); ++itr, ++i) {
			func(i, const_cast<TType&>(*itr));
		}
	}

private:

	virtual void reserve(size_t index) override {
		errs("TUnorderedSet does not have indexing!");
	}

	virtual void resize(size_t index) override {
		errs("TUnorderedSet does not have indexing!");
	}

	virtual void forEachReverse(const std::function<void(size_t, TType&)>& func) override {
		errs("TUnorderedSet cannot be iterated in reverse!");
	}

	struct Hasher {
		size_t operator()(const TType& p) const noexcept {
			return getHash(p);
		}
	};

	std::unordered_set<TType, Hasher> m_Container;
};
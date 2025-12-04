#pragma once

#include <unordered_map>
#include "basic/containers/Container.h"

template <typename TKeyType, typename TValueType>
struct TMultiMap : TContainer<TValueType, const TKeyType&> {

	virtual const size_t getSize() const override {
		return m_Container.size();
	}

	virtual TValueType& get(const TKeyType& index) override {
		return m_Container.find(index)->second;
	}

	virtual const TValueType& get(const TKeyType& index) const override {
		return m_Container.find(index)->second;
	}

	TValueType& addDefaulted(const TKeyType& index) {
		set(index, {});
		return get(index);
	}

	virtual void set(const TKeyType& index, const TValueType& obj) override {
		if constexpr (is_copyable_v<TValueType>) {
			m_Container.emplace(index, obj);
		} else {
			errs("Type is not copyable!");
		}
	}

	virtual void set(const TKeyType& index, TValueType&& obj) override {
		if constexpr (is_moveable_v<TValueType>) {
			m_Container.emplace(index, std::move(obj));
		} else {
			errs("Type is not moveable!");
		}
	}

	virtual TValueType& remove(const TKeyType& index) override {
		auto& obj = get(index);
		m_Container.erase(index);
		return obj;
	}

	virtual void forEach(const std::function<void(const TKeyType&, TValueType&)>& func) override {
		for (auto itr = m_Container.begin(); itr != m_Container.end(); ++itr) {
			func(itr->first, itr->second);
		}
	}

private:

	virtual void reserve(size_t index) override {
		errs("TUnorderedMap does not have indexing!");
	}

	virtual void resize(size_t index) override {
		errs("TUnorderedMap does not have indexing!");
	}

	virtual TValueType& addDefaulted() override {
		errs("TUnorderedMap cannot be default constructed without a key!");
	}

	virtual const TKeyType& add(const TValueType& obj) override {
		errs("TUnorderedMap cannot be added to!");
	}

	virtual const TKeyType& add(TValueType&& obj) override {
		errs("TUnorderedMap cannot be added to!");
	}

	virtual void forEachReverse(const std::function<void(const TKeyType&, TValueType&)>& func) override {
		errs("TUnorderedMap cannot be iterated in reverse!");
	}

	struct Hasher {
		size_t operator()(const TKeyType& p) const noexcept {
			return getHash(p);
		}
	};

	std::unordered_multimap<TKeyType, TValueType, Hasher> m_Container;
};
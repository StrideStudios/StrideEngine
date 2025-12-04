#pragma once

#include <map>
#include "basic/containers/Container.h"

template <typename TKeyType, typename TValueType>
struct TPriorityMap : TContainer<TValueType, const TKeyType&> {

	virtual const size_t getSize() const override {
		return m_Container.size();
	}

	virtual TValueType& get(const TKeyType& index) override {
		return m_Container.at(index);
	}

	virtual const TValueType& get(const TKeyType& index) const override {
		return m_Container.at(index);
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

	virtual void forEachReverse(const std::function<void(const TKeyType&, TValueType&)>& func) override {
		for (auto itr = m_Container.rbegin(); itr != m_Container.rend(); ++itr) {
			func(itr->first, itr->second);
		}
	}

private:

	virtual void reserve(size_t index) override {
		errs("TMap does not have indexing!");
	}

	virtual void resize(size_t index) override {
		errs("TMap does not have indexing!");
	}

	virtual TValueType& addDefaulted() override {
		errs("TMap cannot be default constructed without a key!");
	}

	virtual const TKeyType& add(const TValueType& obj) override {
		errs("TMap cannot be added to!");
	}

	virtual const TKeyType& add(TValueType&& obj) override {
		errs("TMap cannot be added to!");
	}

	std::map<TKeyType, TValueType> m_Container;
};
#pragma once

#include <forward_list>

#include "basic/core/Common.h"
#include "basic/core/Object.h"
#include "sstl/List.h"
#include "sstl/Memory.h"

/*
 * Stores pointers to CObject so they can be automatically deallocated when flush is called
 * This means initialization can be done in a local context to remove clutter
 */

template <typename TRequiredType>
class TResourceManager : public IDestroyable {

public:

	using Storage = TList<TUnique<SObject>>;

	// Flush resources if out of scope
	virtual ~TResourceManager() override {
		TResourceManager::flush();
	}

	Storage& getObjects() {
		return m_Objects;
	}

	const Storage& getObjects() const {
		return m_Objects;
	}

	template <typename TType>
	requires std::is_base_of_v<TRequiredType, TType>
	void create(TType*& outType) {
		m_Objects.push(TUnique<TType>{});
		outType = dynamic_cast<TType*>(m_Objects.bottom().get());
		if constexpr (std::is_base_of_v<IInitializable, TType>) {
			outType->init();
		}
	}

	template <typename TTargetType, typename TType>
	requires (!std::is_same_v<TTargetType, TType>) and std::is_base_of_v<TRequiredType, TTargetType>
	void create(TType*& outType) {
		create<TTargetType>(reinterpret_cast<TTargetType*&>(outType));
	}

	template <typename TType, typename... TArgs>
	requires std::is_base_of_v<TRequiredType, TType> and std::is_constructible_v<TType, TArgs...>
	void create(TType*& outType, TArgs&&... args) {
		m_Objects.push(TUnique<TType>{args...});
		outType = dynamic_cast<TType*>(m_Objects.bottom().get());
		if constexpr (std::is_base_of_v<IInitializable, TType>) {
			outType->init();
		}
	}

	template <typename TType, typename... TArgs>
	requires std::is_base_of_v<TRequiredType, TType> and std::is_base_of_v<TInitializable<TArgs...>, TType> and (not std::is_constructible_v<TType, TArgs...>)
	void create(TType*& outType, TArgs&&... args) {
		m_Objects.push(TUnique<TType>{});
		outType = dynamic_cast<TType*>(m_Objects.bottom().get());
		outType->init(args...);
	}

	template <typename TTargetType, typename TType, typename... TArgs>
	requires (!std::is_same_v<TTargetType, TType>) and std::is_base_of_v<TRequiredType, TTargetType>
	void create(TType*& outType, TArgs&&... args) {
		create<TTargetType, TArgs...>(reinterpret_cast<TTargetType*&>(outType), args...);
	}

	template <typename TType>
	//requires std::is_base_of_v<TRequiredType, TType>
	size_t pushUnique(TUnique<TType>&& inType) {
		return m_Objects.push(std::forward<TUnique<TType>>(inType));
	}

	/*template <typename TType>
	requires std::is_base_of_v<TRequiredType, TType>
	Iterator push(TType* inType) {
		m_Objects.emplace_back(TUnique<TempHelper>{inType});
		inType->itr = m_Objects.begin();
		std::advance(inType->itr, (m_Objects.size() - 1));
		return inType->itr;
	}*/

	// Reverse iterate and destroy
	virtual void flush() {
		m_Objects.forEachReverse([](size_t index, TUnique<SObject>& obj) {
			if (const auto destroyable = dynamic_cast<IDestroyable*>(obj.get())) {
				destroyable->destroy();
			}
		});
		m_Objects.clear();
	}

	template <typename TType>
	requires std::is_base_of_v<TRequiredType, TType>
	void remove(TType* inType) {
		if (inType == nullptr) return;
		if (const auto& destroyable = dynamic_cast<IDestroyable*>(inType->get())) {
			destroyable->destroy();
		}
		m_Objects.pop(m_Objects.find(inType));
	}

	/*virtual void remove(const Iterator& itr) {
		if (itr._Myproxy == nullptr || itr._Getcont() == nullptr) return; // Ensure itr is valid
		if (const auto& destroyable = dynamic_cast<IDestroyable*>(itr->get())) {
			destroyable->destroy();
		}
		m_Objects.erase(itr);
	}*/

	template <typename TType>
	requires std::is_base_of_v<TRequiredType, TType>
	void ignore(TType*& inType) {
		if (inType == nullptr) return;
		m_Objects.pop(m_Objects.find(inType));
	}

	/*virtual void ignore(const Iterator& itr) {
		if (itr._Myproxy == nullptr || itr._Getcont() == nullptr) return; // Ensure itr is valid
		m_Objects.erase(itr);
	}*/

protected:

	virtual void destroy() override {
		flush();
	}

	Storage m_Objects;
};

class CResourceManager : public TResourceManager<SObject> {

public:

	// Gives the ability to call a function from a specific destruction point
	// std::function is pretty slow, so use with caution (so not at runtime)
	struct Callback final : SObject, IDestroyable {
		Callback() = delete;
		Callback(const std::function<void()>& inCallback): mCallback(inCallback) {}
		virtual void destroy() override { mCallback(); }
		std::function<void()> mCallback;
	};

	EXPORT static CResourceManager& get();

	// Calls the function when it is indicated to be destroyed
	void callback(const std::function<void()>& inFunction) {
		Callback* cb;
		create(cb, inFunction);
	}
};
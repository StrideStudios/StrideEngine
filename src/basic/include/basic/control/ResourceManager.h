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

	using Storage = std::list<TUnique<SObject>>;
	using Iterator = Storage::iterator;

	// TODO: until everything is migrated, we use this to ensure no leaks
	struct TempHelper : SObject, IDestroyable {
		TempHelper(TRequiredType* obj): obj(obj) {}
		virtual void destroy() override {
			if (const auto destroyable = dynamic_cast<IDestroyable*>(obj)) {
				destroyable->destroy();
			}
			delete obj;
		}
		TRequiredType* obj;
	};

	// Flush resources if out of scope
	virtual ~TResourceManager() override {
		TResourceManager::flush();
	}

	const Storage& getObjects() const {
		return m_Objects;
	}

	template <typename TType>
	requires std::is_base_of_v<TRequiredType, TType>
	Iterator create(TType*& outType) {
		outType = new TType();
		if constexpr (std::is_base_of_v<IInitializable, TType>) {
			outType->init();
		}
		return push(outType);
	}

	template <typename TTargetType, typename TType>
	requires (!std::is_same_v<TTargetType, TType>) and std::is_base_of_v<TRequiredType, TTargetType>
	Iterator create(TType*& outType) {
		return create<TTargetType>(reinterpret_cast<TTargetType*&>(outType));
	}

	template <typename TType, typename... TArgs>
	requires std::is_base_of_v<TRequiredType, TType> and std::is_constructible_v<TType, TArgs...>
	Iterator create(TType*& outType, TArgs&&... args) {
		outType = new TType(args...);
		if constexpr (std::is_base_of_v<IInitializable, TType>) {
			outType->init();
		}
		return push(outType);
	}

	template <typename TType, typename... TArgs>
	requires std::is_base_of_v<TRequiredType, TType> and std::is_base_of_v<TInitializable<TArgs...>, TType> and (not std::is_constructible_v<TType, TArgs...>)
	Iterator create(TType*& outType, TArgs&&... args) {
		outType = new TType();
		outType->init(args...);
		return push(outType);
	}

	template <typename TTargetType, typename TType, typename... TArgs>
	requires (!std::is_same_v<TTargetType, TType>) and std::is_base_of_v<TRequiredType, TTargetType>
	Iterator create(TType*& outType, TArgs&&... args) {
		return create<TTargetType, TArgs...>(reinterpret_cast<TTargetType*&>(outType), args...);
	}

	template <typename TType>
	requires std::is_base_of_v<TRequiredType, TType>
	Iterator push(TType* inType) {
		m_Objects.emplace_back(TUnique<TempHelper>{inType});
		inType->itr = m_Objects.begin();
		std::advance(inType->itr, (m_Objects.size() - 1));
		return inType->itr;
	}

	// Reverse iterate and destroy
	virtual void flush() {
		for (auto itr = m_Objects.rbegin(); itr != m_Objects.rend(); ++itr) {
			if (const auto destroyable = dynamic_cast<IDestroyable*>(itr->get())) {
				destroyable->destroy();
			}
		}
		m_Objects.clear();
	}

	template <typename TType>
	requires std::is_base_of_v<TRequiredType, TType>
	void remove(TType* inType) {
		if (inType == nullptr) return;
		remove(inType->itr);
	}

	virtual void remove(const Iterator& itr) {
		if (itr._Myproxy == nullptr || itr._Getcont() == nullptr) return; // Ensure itr is valid
		if (const auto& destroyable = dynamic_cast<IDestroyable*>(itr->get())) {
			destroyable->destroy();
		}
		m_Objects.erase(itr);
	}

	template <typename TType>
	requires std::is_base_of_v<TRequiredType, TType>
	void ignore(TType*& inType) {
		ignore(inType->itr);
	}

	virtual void ignore(const Iterator& itr) {
		if (itr._Myproxy == nullptr || itr._Getcont() == nullptr) return; // Ensure itr is valid
		m_Objects.erase(itr);
	}

	TRequiredType*& operator[](size_t index) {
		Iterator itr = m_Objects.begin();
		std::advance(itr, index);
		return *itr;
	}

	const TRequiredType*& operator[](size_t index) const {
		Storage::const_iterator itr = m_Objects.begin();
		std::advance(itr, index);
		return *itr;
	}

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
	Iterator callback(const std::function<void()>& inFunction) {
		Callback* cb;
		return create(cb, inFunction);
	}
};
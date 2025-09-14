#pragma once

#include <deque>

#include "core/Common.h"
#include "core/Object.h"

struct IDestroyable {
	virtual ~IDestroyable() = default;

	virtual void destroy() {}
};

struct SInitializable {};

template <typename... TArgs>
struct TInitializable : SInitializable {
	virtual void init(TArgs... args) {}
};

// typedef which makes it easy to check if a class has an empty init function
typedef TInitializable<> IInitializable;

/*
 * Stores pointers to CObject so they can be automatically deallocated when flush is called
 * This means initialization can be done in a local context to remove clutter
 */
class CResourceManager {

public:

	// Gets a global version of the resource manager that is destroyed upon engine stop
	EXPORT static CResourceManager& get();

	// Flush resources if out of scope
	virtual ~CResourceManager() {
		CResourceManager::flush();
	}

	template <typename TType>
	requires std::is_base_of_v<SObject, TType>
	void create(TType*& outType) {
		outType = new TType();
		if constexpr (std::is_base_of_v<IInitializable, TType>) {
			outType->init();
		}
		push(outType);
	}

	template <typename TTargetType, typename TType>
	requires (!std::is_same_v<TTargetType, TType>) and std::is_base_of_v<SObject, TTargetType>
	void create(TType*& outType) {
		create<TTargetType>(reinterpret_cast<TTargetType*&>(outType));
	}

	template <typename TType, typename... TArgs>
	requires std::is_base_of_v<SObject, TType>
	void create(TType*& outType, TArgs&&... args) {
		if constexpr (std::is_base_of_v<SInitializable, TType>) {
			outType = new TType();
			outType->init(args...);
		} else {
			outType = new TType(args...);
		}
		push(outType);
	}

	template <typename TTargetType, typename TType, typename... TArgs>
	requires (!std::is_same_v<TTargetType, TType>) and std::is_base_of_v<SObject, TTargetType>
	void create(TType*& outType, TArgs&&... args) {
		create<TTargetType, TArgs...>(reinterpret_cast<TTargetType*&>(outType), args...);
	}

	template <typename TType>
	requires std::is_base_of_v<SObject, TType>
	void push(TType* inType) {
		m_Objects.push_back(inType);
	}

	// Reverse iterate and destroy
	virtual void flush() {
		for (auto itr = m_Objects.rbegin(); itr != m_Objects.rend(); ++itr) {
			if (const auto& destroyable = dynamic_cast<IDestroyable*>(*itr)) {
				destroyable->destroy();
			}
			delete *itr;
		}
		m_Objects.clear();
	}

private:

	std::deque<SObject*> m_Objects;
};
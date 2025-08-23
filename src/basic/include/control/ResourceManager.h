#pragma once

#include <deque>

#include "core/Common.h"

struct IDestroyable {
	virtual ~IDestroyable() = default;

	virtual void destroy() {}
};

// class which makes it easy to check if a class extends Initializable
struct SInitializable {};

template <typename... TArgs>
struct IInitializable : SInitializable {
	virtual void init(TArgs... args) {}
};

/*
 * Stores pointers to IDestroyable so they can be automatically deallocated when flush is called
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
	requires std::is_base_of_v<IDestroyable, TType>
	void create(TType*& outType) {
		outType = new TType();
		if constexpr (std::is_base_of_v<SInitializable, TType>) {
			outType->init();
		}
		push(outType);
	}

	template <typename TTargetType, typename TType>
	requires !std::is_same_v<TTargetType, TType> and std::is_base_of_v<IDestroyable, TTargetType>
	void create(TType*& outType) {
		create<TTargetType>(reinterpret_cast<TTargetType*&>(outType));
	}

	template <typename TType, typename... TArgs>
	requires std::is_base_of_v<IDestroyable, TType>
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
	requires !std::is_same_v<TTargetType, TType> and std::is_base_of_v<IDestroyable, TTargetType>
	void create(TType*& outType, TArgs&&... args) {
		create<TTargetType, TArgs...>(reinterpret_cast<TTargetType*&>(outType), args...);
	}

	template <typename TType>
	requires std::is_base_of_v<IDestroyable, TType>
	void push(TType* inType) {
		m_Destroyables.push_back(inType);
	}

	// Reverse iterate and destroy
	virtual void flush() {
		for (auto itr = m_Destroyables.rbegin(); itr != m_Destroyables.rend(); ++itr) {
			(*itr)->destroy();
			delete *itr;
		}
		m_Destroyables.clear();
	}

private:

	std::deque<IDestroyable*> m_Destroyables;
};
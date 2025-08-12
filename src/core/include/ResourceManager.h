#pragma once

#include <deque>

#include "Common.h"

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

	// Flush resources if out of scope
	virtual ~CResourceManager() {
		CResourceManager::flush();
	}

	template <typename TType>
	requires std::is_base_of_v<IDestroyable, TType>
	void push(TType*& outType) {
		outType = new TType();
		if constexpr (std::is_base_of_v<SInitializable, TType>) {
			outType->init();
		}
		m_Destroyables.push_back(outType);
	}

	template <typename TTargetType, typename TType>
	requires !std::is_same_v<TTargetType, TType> and std::is_base_of_v<IDestroyable, TTargetType>
	void push(TType*& outType) {
		push<TTargetType>(reinterpret_cast<TTargetType*&>(outType));
	}

	template <typename TType, typename... TArgs>
	requires std::is_base_of_v<IDestroyable, TType>
	void push(TType*& outType, TArgs&&... args) {
		if constexpr (std::is_base_of_v<SInitializable, TType>) {
			outType = new TType();
			outType->init(args...);
		} else {
			outType = new TType(args...);
		}
		m_Destroyables.push_back(outType);
	}

	template <typename TTargetType, typename TType, typename... TArgs>
	requires !std::is_same_v<TTargetType, TType> and std::is_base_of_v<IDestroyable, TTargetType>
	void push(TType*& outType, TArgs&&... args) {
		push<TTargetType, TArgs...>(reinterpret_cast<TTargetType*&>(outType), args...);
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
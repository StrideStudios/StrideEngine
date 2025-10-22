#pragma once

#include <deque>

#include "basic/core/Common.h"
#include "basic/core/Object.h"
#include "basic/core/Singleton.h"

/*
 * Stores pointers to CObject so they can be automatically deallocated when flush is called
 * This means initialization can be done in a local context to remove clutter
 */
//TODO: allow CResourceManager to add itself for recursive flushing (Specifically with CEditorSpritePass)
class CResourceManager : public SBase, public IDestroyable {

public:

	// Gives the ability to call a function from a specific destruction point
	// std::function is pretty slow, so use with caution (so not at runtime)
	struct Callback final : SBase, IDestroyable {
		Callback() = delete;
		Callback(const std::function<void()>& inCallback): mCallback(inCallback) {}
		virtual void destroy() override { mCallback(); }
		std::function<void()> mCallback;
	};

	using Storage = std::deque<SBase*>;
	using Iterator = Storage::iterator;

	EXPORT static CResourceManager& get();

	// Flush resources if out of scope
	virtual ~CResourceManager() override {
		CResourceManager::flush();
	}

	template <typename TType>
	requires std::is_base_of_v<SBase, TType>
	Iterator create(TType*& outType) {
		outType = new TType();
		if constexpr (std::is_base_of_v<IInitializable, TType>) {
			outType->init();
		}
		return push(outType);
	}

	template <typename TTargetType, typename TType>
	requires (!std::is_same_v<TTargetType, TType>) and std::is_base_of_v<SBase, TTargetType>
	Iterator create(TType*& outType) {
		return create<TTargetType>(reinterpret_cast<TTargetType*&>(outType));
	}

	template <typename TType, typename... TArgs>
	requires std::is_base_of_v<SBase, TType>
	Iterator create(TType*& outType, TArgs&&... args) {
		if constexpr (std::is_base_of_v<SInitializable, TType>) {
			outType = new TType();
			outType->init(args...);
		} else {
			outType = new TType(args...);
		}
		return push(outType);
	}

	template <typename TTargetType, typename TType, typename... TArgs>
	requires (!std::is_same_v<TTargetType, TType>) and std::is_base_of_v<SBase, TTargetType>
	Iterator create(TType*& outType, TArgs&&... args) {
		return create<TTargetType, TArgs...>(reinterpret_cast<TTargetType*&>(outType), args...);
	}

	template <typename TType>
	requires std::is_base_of_v<SBase, TType>
	Iterator push(TType* inType) {
		m_Objects.push_back(inType);
		return m_Objects.begin() + (m_Objects.size() - 1);
	}

	// Calls the function when it is indicated to be destroyed
	Iterator callback(std::function<void()> inFunction) {
		Callback* cb;
		return create(cb, inFunction);
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

	template <typename TType>
	requires std::is_base_of_v<SBase, TType>
	void remove(TType*& inType) {
		if (inType == nullptr) return;

		for (auto itr = m_Objects.begin(); itr != m_Objects.end(); ++itr) {
			if (*itr == nullptr) continue;
			if (*itr == inType) {
				remove(itr);
				inType = nullptr;
			}
		}
	}

	void remove(const Iterator& itr) {
		if (itr._Myproxy == nullptr || itr._Getcont() == nullptr) return; // Ensure itr is valid
		const auto object = *itr;
		m_Objects.erase(itr);
		if (const auto& destroyable = dynamic_cast<IDestroyable*>(object)) {
			destroyable->destroy();
		}
		delete object;
	}

private:

	virtual void destroy() override {
		flush();
	}

	Storage m_Objects;
};
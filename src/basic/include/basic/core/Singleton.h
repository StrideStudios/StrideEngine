#pragma once

#include "sstl/Map.h"
#include "sstl/Memory.h"

#include "basic/core/Common.h"

#define MAKE_SINGLETON(n) \
	private: \
		STATIC_C_BLOCK( \
			initSingleton<n>(#n); \
		) \
	public: \
		static n& get() { \
			return *dynamic_cast<n*>(getSingletons().get(#n).get()); \
		} \
	private:

// A singleton that is initialized upon first call to get()
#define MAKE_LAZY_SINGLETON(n) \
	private: \
		typedef n& FSingletonCallback(); \
		inline static FSingletonCallback* singletonCallback = nullptr; \
		STATIC_C_BLOCK( \
			singletonCallback = [] -> n& { \
				initSingleton<n>(#n); \
				/*
				Sets itself to a simple getter so that there is no branching at runtime
				Since Functions are just memory addresses anyway, it should be nearly as quick as a direct call
				*/ \
				singletonCallback = [] -> n& {return *dynamic_cast<n*>(getSingletons().get(#n).get());}; \
				return (*singletonCallback)(); \
			}; \
		) \
	public: \
		static n& get() { \
			return (*singletonCallback)(); \
		} \
	private:

#define CUSTOM_SINGLETON(n, ...) \
	private: \
		inline static std::string __singleton_name = (std::string(#n) + __VA_ARGS__); \
		STATIC_C_BLOCK( \
			initSingleton<n>(__singleton_name); \
		) \
	public: \
		static n& get() { \
			return *dynamic_cast<n*>(getSingletons().get(__singleton_name).get()); \
		} \
	private:

EXPORT TMap<std::string, TUnique<SObject>>& getSingletons();

template <typename TType>
requires std::is_base_of_v<SObject, TType>
void initSingleton(const std::string& inName) {
	if (!getSingletons().contains(inName)) {
		getSingletons().push(inName, TUnique<TType>{});
		if constexpr (std::is_base_of_v<IDestroyable, TType>) {
			auto destroyable = dynamic_cast<IDestroyable*>(getSingletons().get(inName).get());
			CResourceManager::get().callback([destroyable] {
				destroyable->destroy();
			});
		}
		if constexpr (std::is_base_of_v<IInitializable, TType>) {
			dynamic_cast<IInitializable*>(getSingletons().get(inName).get())->init();
		}
	}
}
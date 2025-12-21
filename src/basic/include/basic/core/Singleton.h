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
		static TShared<n> get() { \
			return getSingletons().get(#n).staticCast<n>(); \
		} \
	private:

// A singleton that is initialized upon first call to get()
#define MAKE_LAZY_SINGLETON(n) \
	private: \
		typedef TShared<n> FSingletonCallback(); \
		inline static FSingletonCallback* singletonCallback = nullptr; \
		STATIC_C_BLOCK( \
			singletonCallback = [] -> TShared<n> { \
				initSingleton<n>(#n); \
				/*
				Sets itself to a simple getter so that there is no branching at runtime
				Since Functions are just memory addresses anyway, it should be nearly as quick as a direct call
				*/ \
				singletonCallback = [] -> TShared<n> {return getSingletons().get(#n).staticCast<n>();}; \
				return (*singletonCallback)(); \
			}; \
		) \
	public: \
		static TShared<n> get() { \
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
		static TShared<n> get() { \
			return getSingletons().get(__singleton_name).staticCast<n>(); \
		} \
	private:

EXPORT TMap<std::string, TShared<SObject>>& getSingletons();
//TODO: big issue with destroy order... NOT FUN

template <typename TType>
requires std::is_base_of_v<SObject, TType>
void initSingleton(const std::string& inName) {
	if (!getSingletons().contains(inName)) {
		auto outPointer = TShared<TType>{};
		getSingletons().push(inName, outPointer.template staticCast<SObject>());
		if constexpr (std::is_base_of_v<IDestroyable, TType>) {
			CResourceManager::get().callback([outPointer] {
				outPointer->destroy();
			});
		}
		if constexpr (std::is_base_of_v<IInitializable, TType>) {
			outPointer->init();
		}
	}
}
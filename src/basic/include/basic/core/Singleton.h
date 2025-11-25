#pragma once

#include <map>

#include "basic/core/Common.h"

#define MAKE_SINGLETON(n) \
	private: \
		STATIC_C_BLOCK( \
			initSingleton<n>(#n); \
		) \
	public: \
		static n& get() { \
			return *static_cast<n*>(getSingleton(#n)); \
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
				singletonCallback = [] -> n& {return *static_cast<n*>(getSingleton(#n));}; \
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
			return *static_cast<n*>(getSingleton(__singleton_name)); \
		} \
	private:

EXPORT bool doesSingletonExist(const std::string& inName);

EXPORT void* getSingleton(const std::string& inName);

EXPORT void addSingleton(const std::string& inName, void* inValue);

template <typename TType>
void initSingleton(const std::string& inName) {
	if (!doesSingletonExist(inName)) {
		TType* object = new TType();
		if constexpr (std::is_base_of_v<SObject, TType>) {
			CResourceManager::get().push(object);
		}
		addSingleton(inName, object);
		if constexpr (std::is_base_of_v<IInitializable, TType>) {
			object->init();
		}
	}
}
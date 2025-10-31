#pragma once

#include <map>

#include "basic/core/Common.h"

#define MAKE_SINGLETON(n) \
	private: \
		STATIC_C_BLOCK( \
			if (!doesSingletonExist(#n)) { \
				n* object; \
				CResourceManager::get().create<n>(object); \
				addSingleton(#n, object); \
			} \
		) \
	public: \
		static n& get() { \
			return *static_cast<n*>(getSingleton(#n)); \
		} \
	private:

// A singleton that is initialized upon first call to get()
#define MAKE_LAZY_SINGLETON(n) \
	public: \
		static n& get() { \
			return (*cb)(); \
		} \
	private: \
	typedef n& FSingletonCallback(); \
	inline static FSingletonCallback* cb = nullptr; \
	STATIC_C_BLOCK( \
		n::cb = [] -> n& { \
			if (!doesSingletonExist(#n)) { \
				n* object = new n(); \
				CResourceManager::get().push(object); \
				addSingleton(#n, object); \
				if constexpr (std::is_base_of_v<IInitializable, n>) { \
					object->init(); \
				} \
			} \
			n::cb = [] -> n& {return *static_cast<n*>(getSingleton(#n));}; \
			return *static_cast<n*>(getSingleton(#n)); \
		}; \
	)

#define CUSTOM_SINGLETON(n, ...) \
	private: \
		inline static std::string __singleton_name = (std::string(#n) + __VA_ARGS__); \
		STATIC_C_BLOCK( \
			if (!doesSingletonExist(__singleton_name)) { \
				n* object; \
				CResourceManager::get().create<n>(object); \
				addSingleton(__singleton_name, object); \
			} \
		) \
	public: \
		static n& get() { \
			return *static_cast<n*>(getSingleton(__singleton_name)); \
		} \
	private:

EXPORT bool doesSingletonExist(const std::string& inName);

EXPORT void* getSingleton(const std::string& inName);

EXPORT void addSingleton(const std::string& inName, void* inValue);
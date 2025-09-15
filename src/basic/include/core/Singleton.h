#pragma once

#include <map>

#include "core/Common.h"

#define MAKE_SINGLETON(n) \
	private: \
		typedef TSingleton<n> n##Singleton; \
	public: \
		static n& get() { \
			return n##Singleton::get(); \
		} \
	private:

EXPORT std::map<std::string, std::shared_ptr<void>>& getSingletons();

// Lazy initialized and safe way to create singletons
template <typename TType>
class TSingleton {

	// Typeid does not need consistency across platforms, just while running
	static const char* getTypeName() {
		return typeid(TType).name();
	}

public:

	static bool contains() {
		return getSingletons().contains(getTypeName());
	}

	static TType& get() {
		if (!contains()) {
			getSingletons().insert(std::make_pair(getTypeName(), std::make_shared<TType>()));
		}
		return *static_cast<TType*>(getSingletons().at(getTypeName()).get());
	}
};
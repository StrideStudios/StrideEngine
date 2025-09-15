#pragma once

#include <map>

#include "core/Common.h"

#define MAKE_SINGLETON(n) \
	STATIC_BLOCK( \
		if (!getSingletons().contains(#n)) getSingletons().insert(std::make_pair(#n, std::make_shared<n>())); \
	) \
	public: \
		static n& get() { \
			return *static_cast<n*>(getSingletons().at(#n).get()); \
		} \
	private:

#define MAKE_LAZY_SINGLETON(n) \
	public: \
		static n& get() { \
			if (!getSingletons().contains(#n)) getSingletons().insert(std::make_pair(#n, std::make_shared<n>())); \
			return *static_cast<n*>(getSingletons().at(#n).get()); \
		} \
	private:

EXPORT std::map<std::string, std::shared_ptr<void>>& getSingletons();
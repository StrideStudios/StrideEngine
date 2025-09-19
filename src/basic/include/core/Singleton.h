#pragma once

#include <map>

#include "core/Common.h"

#define MAKE_SINGLETON(n) \
	private: \
		STATIC_BLOCK( \
			if (!getSingletons().contains(#n)) { \
				getSingletons().insert(std::make_pair(#n, std::make_shared<n>())); \
			} \
		) \
	public: \
		static n& get() { \
			return *static_cast<n*>(getSingletons().at(#n).get()); \
		} \
	private:

#define CUSTOM_SINGLETON(n, ...) \
	private: \
		inline static std::string __singleton_name = (std::string(#n) + __VA_ARGS__); \
		STATIC_BLOCK( \
			if (!getSingletons().contains(__singleton_name)) { \
				getSingletons().insert(std::make_pair(__singleton_name, std::make_shared<n>())); \
			} \
		) \
	public: \
		static n& get() { \
			return *static_cast<n*>(getSingletons().at(__singleton_name).get()); \
		} \
	private:

EXPORT std::map<std::string, std::shared_ptr<void>>& getSingletons();
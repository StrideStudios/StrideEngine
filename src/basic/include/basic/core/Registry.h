#pragma once

#include <map>

#include "basic/core/Common.h"
#include "basic/core/Factory.h"

#define REGISTER_OBJ(registryType, n) \
	private: \
		STATIC_C_BLOCK( \
			if (registryType::get()->getObjects().contains(#n)) return; \
			registryType::get()->getObjects().push(#n, TUnique<n>{}); \
		) \
	public: \
		static n& get() { return *dynamic_cast<n*>(registryType::get()->getObjects().get(#n).get()); } \
	private:
//TODO: remove? Kinda useless with singletons
#define _DEFINE_REGISTRY(id, name, ...) \
		inline static constexpr char CONCAT(__static_registry_name, id)[] = #name; \
		typedef TRegistry<__VA_ARGS__, CONCAT(__static_registry_name, id)> name; \
		template class TRegistry<__VA_ARGS__, CONCAT(__static_registry_name, id)>; \

#define DEFINE_REGISTRY(name, ...) \
	_DEFINE_REGISTRY(__COUNTER__, name, __VA_ARGS__)

template <typename TType, const char* TName>
class TRegistry : public SObject {

	CUSTOM_SINGLETON(TRegistry, TName)

public:

	TMap<std::string, TType>& getObjects() {
		return m_Objects;
	}

	const TMap<std::string, TType>& getObjects() const {
		return m_Objects;
	}

private:

	TMap<std::string, TType> m_Objects;

};
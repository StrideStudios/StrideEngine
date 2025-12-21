#pragma once

#include <map>

#include "basic/core/Common.h"
#include "basic/core/Factory.h"

#define REGISTER_OBJ(registryType, n) \
	private: \
		STATIC_C_BLOCK( \
			if (registryType::get()->getObjects().contains(#n)) return; \
			n* object; \
			CResourceManager::get().create(object); \
			registryType::get()->getObjects().push(#n, object); \
		) \
	public: \
		static n& get() { return *dynamic_cast<n*>(registryType::get()->getObjects().get(#n)); } \
	private:
//TODO: remove? Kinda useless with singletons
#define _DEFINE_REGISTRY(id, name, ...) \
		inline static constexpr char CONCAT(__static_registry_name, id)[] = #name; \
		typedef TRegistry<__VA_ARGS__, CONCAT(__static_registry_name, id)> name; \
		template class TRegistry<__VA_ARGS__, CONCAT(__static_registry_name, id)>; \

#define DEFINE_REGISTRY(name, ...) \
	_DEFINE_REGISTRY(__COUNTER__, name, __VA_ARGS__)

#define _DEFINE_DEFERRED_REGISTRY(id, name, ...) \
		inline static constexpr char CONCAT(__static_deferred_registry_name, id)[] = #__VA_ARGS__; \
		typedef TDeferredRegistry<__VA_ARGS__, CONCAT(__static_deferred_registry_name, id)> name; \
		template class TDeferredRegistry<__VA_ARGS__, CONCAT(__static_deferred_registry_name, id)>; \
		template class TDeferredFactory<__VA_ARGS__, CONCAT(__static_deferred_registry_name, id)>;

// Since Deferred Registry internally relies on a factory, we force the factory to initialize its template
#define DEFINE_DEFERRED_REGISTRY(name, ...) \
	_DEFINE_DEFERRED_REGISTRY(__COUNTER__, name, __VA_ARGS__)

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

// Registry with the intent to have its objects initialized in a deferred manner
// This can be done at any point the user wishes
// It does this by storing registration, and doing it all at once at any point
template <typename TType, const char* TName, typename... TArgs>
class TDeferredRegistry : public SObject  {

	CUSTOM_SINGLETON(TDeferredRegistry, TName)

	typedef TDeferredFactory<TType, TName, TArgs...> TTypeDeferredFactory;

public:

	/*static void init(class CResourceManager& inResourceManager, TArgs... args) {
		for (auto& [name, object] : TTypeDeferredFactory::get().m_Objects) {
			get().m_Objects.insert(std::make_pair(name, TTypeDeferredFactory::construct(name.c_str(), inResourceManager, args...)));
		}
	}*/

	template <typename TChildType = TType>
	requires std::is_base_of_v<TType, TChildType>
	static void registerObject(const char* inName) {
		if (contains(inName)) return;
		TTypeDeferredFactory::template addToFactory<TChildType>(inName);
	}

	static void forEach(const std::function<void(const std::string&, const TType&)>& inFunction) {
		for (auto& pair : get()->m_Objects) {
			inFunction(pair.first, pair.second);
		}
	}

	static bool contains(const char* inName) {
		return get()->m_Objects.contains(inName);
	}

	template <typename TChildType = TType>
	requires std::is_base_of_v<TType, TChildType>
	static TChildType& get(const char* inName, TArgs... args) {
		if (!contains(inName)) {
			get()->m_Objects.insert(std::make_pair(inName, TTypeDeferredFactory::construct(inName, CResourceManager::get(), args...)));
		}
		return dynamic_cast<TChildType&>(get()->m_Objects[inName]);
	}

	std::map<std::string, TType> m_Objects;
};
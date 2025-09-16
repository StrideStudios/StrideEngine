#pragma once

#include <map>

#include "core/Common.h"
#include "core/Factory.h"
#include "Object.h"
#include "control/ResourceManager.h"

#define REGISTER_OBJ(registryType, n) \
	private: \
		STATIC_BLOCK( \
			registryType::registerObject<n>(#n); \
		) \
	public: \
		static n& get() { return *registryType::get<n>(#n); } \
		static const char* name() { return #n; } \
	private:

#define DEFINE_REGISTRY(n, ...) \
	inline static constexpr char n##Registry##Name[] = #n __VA_ARGS__; \
	typedef TRegistry<n, n##Registry##Name> n##Registry; \
	template class TRegistry<n, n##Registry##Name>;

// Since Deferred Registry internally relies on a factory, we force the factory to initialize its template
#define DEFINE_DEFERRED_REGISTRY(n, ...) \
	inline static constexpr char n##DeferredRegistry##Name[] = #n __VA_ARGS__; \
	typedef TDeferredRegistry<n, n##DeferredRegistry##Name> n##DeferredRegistry; \
	template class TDeferredRegistry<n, n##DeferredRegistry##Name>; \
	template class TDeferredFactory<n, n##DeferredRegistry##Name>;

template <typename TType, const char* TName>
requires std::is_base_of_v<SObject, TType>
class TRegistry {

	CUSTOM_SINGLETON(TRegistry, TName)

public:

	template <typename TChildType = TType>
	//requires std::is_base_of_v<TType, TChildType>
	static void registerObject(const char* inName) {
		if (get().m_Objects.contains(inName)) return;
		auto object = std::make_shared<TChildType>();
		get().m_Objects.insert(std::make_pair(inName, object));
		if (const auto& initializable = std::dynamic_pointer_cast<IInitializable>(object)) {
			initializable->init();
		}
	}

	template <typename TChildType = TType>
	//requires std::is_base_of_v<TType, TChildType>
	static std::shared_ptr<TChildType> get(const char* inName) {
		if (!get().m_Objects.contains(inName)) {
			errs("Could not get registry object {}", inName);
		}
		return std::dynamic_pointer_cast<TChildType>(get().m_Objects[inName]);
	}

	std::map<std::string, std::shared_ptr<TType>> m_Objects;

};

// Registry with the intent to have its objects initialized in a deferred manner
// This can be done at any point the user wishes
// It does this by storing registration, and doing it all at once at any point
template <typename TType, const char* TName, typename... TArgs>
requires std::is_base_of_v<SObject, TType>
class TDeferredRegistry {

	CUSTOM_SINGLETON(TDeferredRegistry, TName)

	typedef TDeferredFactory<TType, TName, TArgs...> TTypeDeferredFactory;

public:

	static void init(CResourceManager& inResourceManager, TArgs... args) {
		for (auto& [name, object] : get().m_Objects) {
			get().m_Objects.insert(std::make_pair(name, TTypeDeferredFactory::construct(name.c_str(), inResourceManager, args...)));
		}
	}

	template <typename TChildType = TType>
	//requires std::is_base_of_v<TType, TChildType>
	static void registerObject(const char* inName) {
		if (get().m_Objects.contains(inName)) return;
		TTypeDeferredFactory::template addToFactory<TChildType>(inName);
	}

	template <typename TChildType = TType>
	//requires std::is_base_of_v<TType, TChildType>
	static TChildType* get(const char* inName) {
		if (!get().m_Objects.contains(inName)) {
			errs("Could not get registry object {}", inName);
		}
		return dynamic_cast<TChildType*>(get().m_Objects[inName]);
	}

	std::map<std::string, TType*> m_Objects;
};
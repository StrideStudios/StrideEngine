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

template <typename TType>
class TTypedRegistry {

	std::map<std::string, TType> m_Objects;

public:

	bool contains(const char* inName) const {
		return m_Objects.contains(inName);
	}

	void registerObject(const char* inName, TType inObject) {
		m_Objects.insert(std::make_pair(inName, inObject));
	}

	void forEachObject(const std::function<void(const std::string&, TType)>& inFunction) const {
		for (auto& [name, object] : m_Objects) {
			inFunction(name, object);
		}
	}

	TType get(const char* inName) {
		if (!contains(inName)) {
			errs("Could not get registry object {}", inName);
		}
		return m_Objects[inName];
	}
};

typedef TTypedRegistry<std::shared_ptr<SObject>> CSharedRegistry;
typedef TTypedRegistry<SObject*> CStandardRegistry;
typedef TTypedRegistry<std::shared_ptr<void>> CStaticRegistry;

template <typename TType, const char* TName>
requires std::is_base_of_v<SObject, TType>
class TRegistry : public CSharedRegistry {

	CUSTOM_SINGLETON(TRegistry, TName)

public:

	TRegistry() {

	}

	template <typename TChildType = TType>
	//requires std::is_base_of_v<TType, TChildType>
	static void registerObject(const char* inName) {
		if (static_cast<CSharedRegistry&>(get()).contains(inName)) return;
		auto object = std::make_shared<TChildType>();
		static_cast<CSharedRegistry&>(get()).registerObject(inName, object);
		if (const auto& initializable = std::dynamic_pointer_cast<IInitializable>(object)) {
			initializable->init();
		}
	}

	static void forEachObject(const std::function<void(std::string, std::shared_ptr<SObject>)>& inFunction) {
		static_cast<CSharedRegistry&>(get()).forEachObject(inFunction);
	}

	template <typename TChildType = TType>
	//requires std::is_base_of_v<TType, TChildType>
	static std::shared_ptr<TChildType> get(const char* inName) {
		return std::dynamic_pointer_cast<TChildType>(static_cast<CSharedRegistry&>(get()).get(inName));
	}
};

// Registry with the intent to have its objects initialized in a deferred manner
// This can be done at any point the user wishes
// It does this by storing registration, and doing it all at once at any point
template <typename TType, const char* TName, typename... TArgs>
requires std::is_base_of_v<SObject, TType>
class TDeferredRegistry : public CStandardRegistry {

	CUSTOM_SINGLETON(TDeferredRegistry, TName)

	typedef TDeferredFactory<TType, TName, TArgs...> TTypeDeferredFactory;

public:

	static void init(CResourceManager& inResourceManager, TArgs... args) {
		TTypeDeferredFactory::forEachObject([&](const std::string& inName) {
			static_cast<CStandardRegistry&>(get()).registerObject(inName.c_str(), TTypeDeferredFactory::construct(inName.c_str(), inResourceManager, args...));
		});
	}

	template <typename TChildType = TType>
	//requires std::is_base_of_v<TType, TChildType>
	static void registerObject(const char* inName) {
		if (static_cast<CStandardRegistry&>(get()).contains(inName)) return;
		TTypeDeferredFactory::template addToFactory<TChildType>(inName);
	}

	static void forEachObject(const std::function<void(const std::string&, SObject*)>& inFunction) {
		static_cast<CStandardRegistry&>(get()).forEachObject(inFunction);
	}

	template <typename TChildType = TType>
	//requires std::is_base_of_v<TType, TChildType>
	static TChildType* get(const char* inName) {
		return dynamic_cast<TChildType*>(static_cast<CStandardRegistry&>(get()).get(inName));
	}
};
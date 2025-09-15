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
	typedef TRegistry<n, n##Registry##Name> n##Registry;

#define DEFINE_DEFERRED_REGISTRY(n, ...) \
	inline static constexpr char n##DeferredRegistry##Name[] = #n __VA_ARGS__; \
	typedef TDeferredRegistry<n, n##DeferredRegistry##Name> n##DeferredRegistry;

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

EXPORT std::unique_ptr<CSharedRegistry>& getSharedRegistry(const char* inName);
EXPORT std::unique_ptr<CStandardRegistry>& getStandardRegistry(const char* inName);

// A registry that holds objects that are always created before main
EXPORT const std::unique_ptr<CStaticRegistry>& getStaticRegistry();

template <typename TType, const char* TName>
requires std::is_base_of_v<SObject, TType>
class TRegistry : public CSharedRegistry {

	static std::unique_ptr<CSharedRegistry>& get() {
		return getSharedRegistry(TName);
	}

public:

	TRegistry() {

	}

	template <typename TChildType = TType>
	//requires std::is_base_of_v<TType, TChildType>
	static void registerObject(const char* inName) {
		if (get()->contains(inName)) return;
		auto object = std::make_shared<TChildType>();
		get()->registerObject(inName, object);
		if (const auto& initializable = std::dynamic_pointer_cast<IInitializable>(object)) {
			initializable->init();
		}
	}

	static void forEachObject(const std::function<void(std::string, std::shared_ptr<SObject>)>& inFunction) {
		get()->forEachObject(inFunction);
	}

	template <typename TChildType = TType>
	//requires std::is_base_of_v<TType, TChildType>
	static std::shared_ptr<TChildType> get(const char* inName) {
		return std::dynamic_pointer_cast<TChildType>(get()->get(inName));
	}
};

// Registry with the intent to have its objects initialized in a deferred manner
// This can be done at any point the user wishes
// It does this by storing registration, and doing it all at once at any point
template <typename TType, const char* TName, typename... TArgs>
requires std::is_base_of_v<SObject, TType>
class TDeferredRegistry : public CStandardRegistry {

	typedef TDeferredFactory<TType, TName, TArgs...> TTypeDeferredFactory;

	static std::unique_ptr<CStandardRegistry>& get() {
		return getStandardRegistry(TName);
	}

public:

	static void init(CResourceManager& inResourceManager, TArgs... args) {
		TTypeDeferredFactory::forEachObject([&](const std::string& inName) {
			get()->registerObject(inName.c_str(), TTypeDeferredFactory::construct(inName.c_str(), inResourceManager, args...));
		});
	}

	template <typename TChildType = TType>
	//requires std::is_base_of_v<TType, TChildType>
	static void registerObject(const char* inName) {
		if (get()->contains(inName)) return;
		TTypeDeferredFactory::template addToFactory<TChildType>(inName);
	}

	static void forEachObject(const std::function<void(const std::string&, SObject*)>& inFunction) {
		get()->forEachObject(inFunction);
	}

	template <typename TChildType = TType>
	//requires std::is_base_of_v<TType, TChildType>
	static TChildType* get(const char* inName) {
		return dynamic_cast<TChildType*>(get()->get(inName));
	}
};
/*
STATIC_BLOCK(
	static constexpr char c[] = "TRegistry<SObject>";
	getStaticRegistry()->registerObject(c, std::make_shared<TRegistry<SObject, c>>());
	auto registry = std::static_pointer_cast<TRegistry<SObject, c>>(getStaticRegistry()->get(c));
)*/
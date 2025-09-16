#pragma once

#include <map>

#include "control/ResourceManager.h"
#include "core/Common.h"
#include "core/Object.h"
#include "core/Singleton.h"

#define ADD_TO_FACTORY(factoryType, n) \
	private: \
		STATIC_BLOCK( \
			factoryType::addToFactory<n>(#n); \
		) \
	public: \
		static const char* name() { return #n; } \
		static std::shared_ptr<n> construct() { return factoryType::construct<n>(#n); } \
	private:

#define DEFINE_FACTORY(n, ...) \
	inline static constexpr char n##Factory##Name[] = #n __VA_ARGS__; \
	typedef TFactory<n, n##Factory##Name> n##Factory; \
	template class TFactory<n, n##Factory##Name>;

#define DEFINE_DEFERRED_FACTORY(n, ...) \
	inline static constexpr char n##DeferredFactory##Name[] = #n __VA_ARGS__; \
	typedef TDeferredFactory<n, n##DeferredFactory##Name> n##DeferredFactory; \
	template class TDeferredFactory<n, n##DeferredFactory##Name>;

template <typename TType, typename... TArgs>
class TTypedFactory {

protected:

	// Function for constructing an object
	typedef TType FConstructor(TArgs...);

private:

	std::map<std::string, FConstructor*> m_Objects;

public:

	bool contains(const char* inName) const {
		return m_Objects.contains(inName);
	}

	bool contains(const std::string& inName) const {
		return m_Objects.contains(inName);
	}

	void addToFactory(const char* inName, FConstructor* inFunction) {
		m_Objects.insert(std::make_pair(inName, inFunction));
	}

	void forEachObject(const std::function<void(const std::string&)>& inFunction) const {
		for (auto& [name, constructor] : m_Objects) {
			inFunction(name);
		}
	}

	TType construct(const char* inName, TArgs... args) {
		if (!m_Objects.contains(inName)) {
			errs("Could not construct object {}", inName);
		}
		return (*m_Objects[inName])(args...);
	}
};

typedef TTypedFactory<std::shared_ptr<SObject>> CSharedFactory;
typedef TTypedFactory<SObject*, CResourceManager&> CStandardFactory;

template <typename TType, const char* TName>
requires std::is_base_of_v<SObject, TType>
class TFactory : public CSharedFactory {

	CUSTOM_SINGLETON(TFactory, TName)

public:

	template <typename TChildType = TType>
	//requires std::is_base_of_v<TType, TChildType>
	static void addToFactory(const char* inName) {
		if (static_cast<CSharedFactory&>(get()).contains(inName)) return;
		static_cast<CSharedFactory&>(get()).addToFactory(inName, [] -> std::shared_ptr<SObject> { return std::make_shared<TChildType>(); });
	}

	template <typename TChildType = TType>
	//requires std::is_base_of_v<TType, TChildType>
	static std::shared_ptr<TChildType> construct(const char* inName) {
		return std::dynamic_pointer_cast<TChildType>(static_cast<CSharedFactory&>(get()).construct(inName));
	}
};

template <typename TType, const char* TName, typename... TArgs>
requires std::is_base_of_v<SObject, TType>
class TDeferredFactory : public CStandardFactory {

	CUSTOM_SINGLETON(TDeferredFactory, TName)

public:

	template <typename TChildType = TType>
	//requires std::is_base_of_v<TType, TChildType>
	static void addToFactory(const char* inName) {
		if (static_cast<CStandardFactory&>(get()).contains(inName)) return;
		static_cast<CStandardFactory&>(get()).addToFactory(inName, [](CResourceManager& inResourceManager, TArgs... args) -> SObject* {
			TChildType* object;
			inResourceManager.create(object, args...);
			return object;
		});
	}

	static void forEachObject(const std::function<void(const std::string&)>& inFunction) {
		static_cast<CStandardFactory&>(get()).forEachObject(inFunction);
	}

	template <typename TChildType = TType>
	//requires std::is_base_of_v<TType, TChildType>
	static TChildType* construct(const char* inName, CResourceManager& inResourceManager, TArgs... args) {
		return dynamic_cast<TChildType*>(static_cast<CStandardFactory&>(get()).construct(inName, inResourceManager, args...));
	}
};
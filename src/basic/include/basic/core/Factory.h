#pragma once

#include <map>

#include "basic/core/Common.h"
#include "basic/core/Singleton.h"

#define ADD_TO_FACTORY(factoryType, n) \
	private: \
		STATIC_BLOCK( \
			factoryType::addToFactory<n>(#n); \
		) \
	public: \
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

template <typename TType, const char* TName>
class TFactory : public SObject {

	CUSTOM_SINGLETON(TFactory, TName)

	typedef std::shared_ptr<TType> FConstructor();

public:

	template <typename TChildType = TType>
	requires std::is_base_of_v<TType, TChildType>
	static void addToFactory(const char* inName) {
		if (get().m_Objects.contains(inName)) return;
		get().m_Objects.insert(std::make_pair(inName, [] -> std::shared_ptr<TType> { return std::make_shared<TChildType>(); }));
	}

	template <typename TChildType = TType>
	requires std::is_base_of_v<TType, TChildType>
	static std::shared_ptr<TChildType> construct(const char* inName) {
		if (!get().m_Objects.contains(inName)) {
			errs("Could not construct object {}", inName);
		}
		return std::dynamic_pointer_cast<TChildType>((*get().m_Objects[inName])());
	}

	std::map<std::string, FConstructor*> m_Objects;

};

template <typename TType, const char* TName, typename... TArgs>
class TDeferredFactory : public SObject {

	CUSTOM_SINGLETON(TDeferredFactory, TName)

	typedef TType* FConstructor(CResourceManager&, TArgs...);

public:

	template <typename TChildType = TType>
	requires std::is_base_of_v<TType, TChildType>
	static void addToFactory(const char* inName) {
		if (get().m_Objects.contains(inName)) return;
		get().m_Objects.insert(std::make_pair(inName, [](CResourceManager& inResourceManager, TArgs... args) -> TType* {
			TChildType* object;
			inResourceManager.create(object, args...);
			return object;
		}));
	}

	template <typename TChildType = TType>
	requires std::is_base_of_v<TType, TChildType>
	static TChildType* construct(const char* inName, CResourceManager& inResourceManager, TArgs... args) {
		if (!get().m_Objects.contains(inName)) {
			errs("Could not get factory object {}", inName);
		}
		return dynamic_cast<TChildType*>((*get().m_Objects[inName])(inResourceManager, args...));
	}

	std::map<std::string, FConstructor*> m_Objects;

};
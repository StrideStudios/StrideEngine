#pragma once

#include <string>
#include <map>
#include <type_traits>

#include "Common.h"

#define ADD_MODULE(type, name) \
	public: \
	struct Registry { Registry() { CModuleManager::addModule<type>(name); } }; \
	inline static Registry reg{}; \
	public: \
	static type& get() { return *CModuleManager::getModule<type>(name); } \
	private:

class CModule : public IInitializable<>, public IDestroyable {};

class EXPORT CModuleManager final {

	std::map<std::string, CModule*> m_Modules;

public:

	template <typename TModule>
	requires std::is_base_of_v<CModule, TModule>
	static void addModule(const std::string& inName) {
		addModule_Internal(std::make_pair(inName, new TModule()));
	}

	template <typename TModule>
	requires std::is_base_of_v<CModule, TModule>
	static TModule* getModule(const std::string& inName) {
		return static_cast<TModule*>(getModule_Internal(inName));
	}

private:

	static void addModule_Internal(std::pair<const std::string&, CModule*> inPair);

	static CModule* getModule_Internal(const std::string& inName);
};
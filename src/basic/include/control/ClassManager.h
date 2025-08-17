#pragma once

#include <map>

#include "core/Common.h"

#define REGISTER_CLASS(n) \
	private: \
	struct Registry { Registry() { CClassManager::registerClass<n>(#n); } }; \
	inline static Registry reg{}; \
	public: \
	virtual std::string getTypeId() const { return #n; } \
	private:

// Gives the ability to construct a class based on a class name
class EXPORT CClassManager final {

	std::map<std::string, std::function<std::shared_ptr<void>()>> m_Classes;

public:

	template <typename TType>
	static void registerClass(const std::string& typeName) {
		register_Internal(std::make_pair(typeName, [] { return std::make_shared<TType>(); }));
	}

	static std::shared_ptr<void> construct(const std::string& inTypeName);

private:

	static void register_Internal(std::pair<const std::string&, std::function<std::shared_ptr<void>()>> inPair);

};
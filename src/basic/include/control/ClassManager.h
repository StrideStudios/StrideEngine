#pragma once

#include <map>

#include "core/Common.h"

#define REGISTER_CLASS(n) \
	private: \
	struct Class final : SClass { \
		Class() { CClassManager::registerClass(#n, this); }; \
		virtual std::shared_ptr<void> construct() const override { return std::make_shared<n>(); } \
		virtual const char* getName() const override { return #n; } \
	}; \
	inline static Class c{}; \
	public: \
	virtual const SClass& getClass() const { return c; } \
	static const SClass& staticClass() { return c; } \
	private:

struct SClass {
	virtual ~SClass() = default;

	virtual std::shared_ptr<void> construct() const = 0;

	virtual const char* getName() const = 0;
};

// Gives the ability to construct a class based on a class name
class CClassManager final {

	friend SClass;

	std::map<std::string, SClass*> m_Classes;

public:

	EXPORT static void registerClass(const char* typeName, SClass* inClass);

	EXPORT static std::shared_ptr<void> construct(const char* inTypeName);

};
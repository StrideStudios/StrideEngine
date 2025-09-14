#pragma once

#include <map>

#include "core/Common.h"
#include "core/Factory.h"
/*
#define REGISTER_CLASS(n) \
	private: \
	struct Class final : SClass { \
		Class() { CClassFactory::registerObject<n>(#n); }; \
		virtual std::shared_ptr<void> construct() const override { return std::make_shared<n>(); } \
		virtual std::string getName() const override { return #n; } \
	}; \
	inline static Class c{}; \
	public: \
	virtual const SClass& getClass() const { return c; } \
	static const SClass& staticClass() { return c; } \
	private:

struct SClass {
	virtual ~SClass() = default;

	virtual std::shared_ptr<void> construct() const = 0;

	virtual std::string getName() const = 0;
};

typedef TFactory<void, "CClass"> CClassFactory;
*/
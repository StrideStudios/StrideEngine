#pragma once

#include <type_traits>
#include <memory>

#include "Registry.h"
#include "core/Object.h"
#include "core/Factory.h"

/*
 * Simple registration for classes and structs
 * For these to work properly, an empty constructor needs to be provided, and it needs to inherit from SObject
 */

#define REGISTER_STRUCT(n, ...) \
	private: \
		typedef TClass<n, __VA_ARGS__> Class; \
		inline static std::shared_ptr<Class> c = nullptr; \
		STATIC_C_BLOCK( c = makeClass<Class>(#n); ) \
	public: \
		virtual std::shared_ptr<SClass> getClass() const override { return c; } \
		static std::shared_ptr<Class> staticClass() { return c; } \

#define REGISTER_CLASS(n, ...) \
	REGISTER_STRUCT(n, __VA_ARGS__) \
	private:

// Represents a object's class
// The purpose of this is to have an easy way to construct a class with just the class's name
struct SClass {

	SClass() = delete;

	SClass(const std::string& inName): m_Name(inName) {}

	virtual ~SClass() = default;

	virtual std::string getName() const {
		return m_Name;
	}

	virtual bool isAbstract() const = 0;

	virtual std::shared_ptr<SObject> construct() const = 0;

	virtual std::shared_ptr<SClass> getParent() const = 0;

	virtual bool doesInherit(const std::shared_ptr<SClass>& inClass) = 0;

	friend bool operator==(const SClass& fst, const SClass& snd) {
		return fst.getName() == snd.getName();
	}

	// Name of the current class
	std::string m_Name;

};

DEFINE_REGISTRY(SClass)

template <typename... TParentClasses>
//requires std::is_base_of_v<SObject, TType>
struct TClass : SClass {

	using Current = SObject;

	TClass(): SClass("None") {}

	virtual std::shared_ptr<SObject> construct() const override {
		return nullptr;
	}

	virtual bool isAbstract() const override {
		return true;
	}

	virtual std::shared_ptr<SClass> getParent() const override {
		return nullptr;
	}

	virtual bool doesInherit(const std::shared_ptr<SClass>& inClass) override {
		return false;
	}

};

template <typename TCurrentClass, typename... TParentClasses>
struct TGenericClass : SClass {

	using Current = TCurrentClass;

	using Super = TClass<TParentClasses...>;

	TGenericClass(const std::string& inName): SClass(inName) {}

	virtual bool isAbstract() const override {
		return std::is_abstract_v<TCurrentClass>;
	}

	virtual std::shared_ptr<SObject> construct() const override {
		if constexpr (std::is_abstract_v<TCurrentClass>) {
			return nullptr;
		} else {
			return std::make_shared<TCurrentClass>();
		}
	}

	virtual std::shared_ptr<SClass> getParent() const override {
		return Super::Current::staticClass();
	}

	virtual bool doesInherit(const std::shared_ptr<SClass>& inClass) override {
		if (inClass == nullptr) return false;
		if (*this == *inClass) {
			return true;
		}
		return getParent()->doesInherit(inClass);
	}
};

template <typename TCurrentClass, typename... TParentClasses>
//requires std::is_base_of_v<SObject, TCurrentClass>
struct TClass<TCurrentClass, TParentClasses...> : TGenericClass<TCurrentClass, TParentClasses...> {

	using Current = TCurrentClass;

	TClass(const std::string& inName): TGenericClass<TCurrentClass, TParentClasses...>(inName) {}

};

template <typename TClassType>
std::shared_ptr<TClassType> makeClass(const std::string& inName) {
	std::shared_ptr<TClassType> c = std::make_shared<TClassType>(inName);
	SClassRegistry::registerObject(inName.c_str(), c);
	return c;
}
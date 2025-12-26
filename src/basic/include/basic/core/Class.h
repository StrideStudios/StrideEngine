#pragma once

#include <type_traits>

#include "Registry.h"
#include "basic/core/Object.h"
#include "basic/control/ResourceManager.h"

/*
 * Simple registration for classes and structs
 * For these to work properly, an empty constructor needs to be provided, and it needs to inherit from SObject
 */

#define REGISTER_STRUCT(n, ...) \
	private: \
		typedef TClass<n, __VA_ARGS__> Class; \
		inline static Class* c = makeClass<Class>(#n); \
	public: \
		typedef __VA_ARGS__ Super; \
		virtual SClass* getClass() const override { return c; } \
		static Class* staticClass() { return c; } \
		TShared<n> asShared() { return getShared().template staticCast<n>(); } \
		TWeak<n> asWeak() { return getWeak().template staticCast<n>(); }

#define REGISTER_CLASS(n, ...) \
	REGISTER_STRUCT(n, __VA_ARGS__) \
	private:

// Represents a object's class
// The purpose of this is to have an easy way to construct a class with just the class's name
struct SClass : SObject {

	SClass() = delete;

	SClass(const std::string& inName): m_Name(inName) {}

	virtual std::string getName() const {
		return m_Name;
	}

	virtual bool isAbstract() const = 0;

	virtual void constructObject(SObject&) const = 0;

	virtual SObject* constructPointer() const = 0;

	template <typename TType>
	void constructObject(TUnique<TType>& object) const {
		object = TUnique<TType>{static_cast<TType*>(constructPointer())};
	}

	template <typename TType>
	void constructObject(TShared<TType>& object) const {
		object = TShared<TType>{static_cast<TType*>(constructPointer())};
	}

	virtual SObject* construct(CResourceManager& inResourceManager) const = 0;

	virtual SClass* getParent() const = 0;

	virtual bool doesInherit(SClass* inClass) = 0;

	// Name of the current class
	std::string m_Name;

};

DEFINE_REGISTRY(SClassRegistry, TUnique<SClass>)

template <typename... TParentClasses>
//requires std::is_base_of_v<SObject, TType>
struct TClass : SClass {

	using Current = SObject;

	TClass(): SClass("None") {}

	virtual void constructObject(SObject& object) const override {}

	virtual SObject* constructPointer() const override { return nullptr; }

	virtual SObject* construct(CResourceManager& inResourceManager) const override {
		return nullptr;
	}

	virtual bool isAbstract() const override {
		return true;
	}

	virtual SClass* getParent() const override {
		return nullptr;
	}

	virtual bool doesInherit(SClass* inClass) override {
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

	virtual void constructObject(SObject& object) const override {
		if constexpr (not std::is_abstract_v<TCurrentClass>) {
			object = TCurrentClass{};
		}
	}

	virtual SObject* constructPointer() const override {
		if constexpr (std::is_abstract_v<TCurrentClass>) {
			return nullptr;
		} else {
			return new TCurrentClass();
		}
	}

	virtual SObject* construct(CResourceManager& inResourceManager) const override {
		if constexpr (std::is_abstract_v<TCurrentClass>) {
			return nullptr;
		} else {
			TCurrentClass* obj;
			inResourceManager.create(obj);
			return obj;
		}
	}

	virtual SClass* getParent() const override {
		// Prevents object class from returning itself as its own parent
		if constexpr (std::is_same_v<TCurrentClass, SObject> and std::is_same_v<typename Super::Current, SObject>) {
			return nullptr;
		} else {
			return Super::Current::staticClass();
		}
	}

	virtual bool doesInherit(SClass* inClass) override {
		if (inClass == nullptr) return false;
		if (getName() == inClass->getName()) {
			return true;
		}
		// If parent class is SObject, then that is the endpoint
		if constexpr (std::is_same_v<typename Super::Current, SObject>) {
			return false;
		} else {
			return getParent()->doesInherit(inClass);
		}
	}
};

template <typename TCurrentClass, typename... TParentClasses>
//requires std::is_base_of_v<SObject, TCurrentClass>
struct TClass<TCurrentClass, TParentClasses...> : TGenericClass<TCurrentClass, TParentClasses...> {

	using Current = TCurrentClass;

	TClass(const std::string& inName): TGenericClass<TCurrentClass, TParentClasses...>(inName) {}

};

template <typename TClassType>
TClassType* makeClass(const std::string& inName) {
	if (!SClassRegistry::get()->getObjects().contains(inName)) {
		SClassRegistry::get()->getObjects().push(inName, TUnique<TClassType>{inName});
	}
	return static_cast<TClassType*>(SClassRegistry::get()->getObjects().get(inName).get());
}
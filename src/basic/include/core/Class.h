#pragma once

#include <type_traits>
#include <memory>

#include "core/Object.h"
#include "core/Factory.h"

/*
 * Simple registration for classes and structs
 * For these to work properly, an empty constructor needs to be provided, and it needs to inherit from SObject
 */

#define REGISTER_STRUCT(n) \
	public: \
		virtual const char* getName() const override { return #n; } \
		static const char* name() { return #n; } \

#define REGISTER_CLASS(n) \
	REGISTER_STRUCT(n) \
	private:

//virtual const Class& getClass() const { return c; }

#define DEFINE_CLASS(n, ...) \
	private: \
		typedef TClass<n, ##__VA_ARGS__> Class; \
		inline static Class c{#n}; \
	public: \
		virtual SClass* getClass() const override { return &c; } \
		static Class* staticClass() { return &c; } \
	private:

DEFINE_FACTORY(SObject)

// Represents a object's class
// The purpose of this is to have an easy way to construct a class with just the class's name
struct SClass {

	virtual const std::string& getClassName() const = 0;

	virtual std::shared_ptr<SObject> construct() const = 0;

	virtual SClass* getParent() const = 0;

	virtual bool doesInherit(const SClass* inClass) = 0;

	friend bool operator==(const SClass& fst, const SClass& snd) {
		return fst.getClassName() == snd.getClassName();
	}

};

template <typename... TParentClasses>
//requires std::is_base_of_v<SObject, TType>
struct TClass : SClass {

	using Current = SObject;

	using Type = TClass<TParentClasses...>;

	virtual SClass* getParent() const override {
		return nullptr;
	};

	virtual bool doesInherit(const SClass* inClass) override {
		return false;
	}

};

template <typename TCurrentClass, typename... TParentClasses>
//requires std::is_base_of_v<SObject, TType>
struct TClass<TCurrentClass, TParentClasses...> : SClass {

	using Current = TCurrentClass;

	using Type = TClass<TCurrentClass, TParentClasses...>;;

	using Super = TClass<TParentClasses...>;

	TClass() = delete;

	TClass(const std::string& inName): m_Name(inName) {}

	virtual std::shared_ptr<SObject> construct() const override {
		if constexpr (std::is_abstract_v<TCurrentClass>) {
			return nullptr;
		} else {
			return std::make_shared<TCurrentClass>();
		}
	}

	virtual const std::string& getClassName() const override {
		return m_Name;
	}

	virtual SClass* getParent() const override {
		return Super::Current::staticClass();
	}

	virtual bool doesInherit(const SClass* inClass) override {
		if (inClass == nullptr) return false;
		if (*this == *inClass) {
			return true;
		}
		return getParent()->doesInherit(inClass);
	}

private:

	// Name of the current class
	std::string m_Name;

};
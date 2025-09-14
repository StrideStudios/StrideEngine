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
		virtual const char* getName() const override { return #n; }

#define REGISTER_CLASS(n) \
	REGISTER_STRUCT(n) \
	private:

DEFINE_FACTORY(SObject)

// Represents a object's class
// The purpose of this is to have an easy way to construct a class with just the class's name
class CClass final {

	// Function for constructing an object
	typedef std::shared_ptr<SObject> (*FConstructor)();

public:

	CClass() = delete;

	CClass(const char* inName, const FConstructor inConstructor): m_Name(inName), m_Constructor(inConstructor) {}

	template <typename TType>
	static CClass make(const char* inName) {
		CClass c(inName, [] -> std::shared_ptr<SObject> { return std::make_shared<TType>(); });
		SObjectFactory::addToFactory<TType>(inName);
		return std::move(c);
	}

	std::shared_ptr<SObject> create() const { return (*m_Constructor)(); }
	const char* getName() const { return m_Name; }

private:

	const char* m_Name;
	FConstructor m_Constructor;

};

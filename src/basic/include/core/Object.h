#pragma once

// A simple base class for all objects
// If a derived class does not register itself, it will always be abstract

struct SClass;

struct SObject {

	//typedef TClass<SObject> Class;
	static SClass* staticClass() {
		return nullptr;
	}

	virtual ~SObject() = default;

	virtual SClass* getClass() const { return nullptr; } \

	virtual const char* getName() const = 0;
};

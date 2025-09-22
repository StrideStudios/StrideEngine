#pragma once

// A simple base class for all objects
// If a derived class does not register itself, it will always be abstract

struct SClass;

struct SObject {

	//typedef TClass<SObject> Class;
	static std::shared_ptr<SClass> staticClass() {
		return nullptr;
	}

	virtual ~SObject() = default;

	virtual std::shared_ptr<SClass> getClass() const = 0;
};

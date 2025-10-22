#pragma once

// A simple base class for all objects
// If a derived class does not register itself, it will always be abstract

struct SClass;

struct SBase {
	virtual ~SBase() = default;
};

struct SObject : SBase {

	virtual ~SObject() = default;

	virtual std::shared_ptr<SClass> getClass() const {
		return staticClass();
	}

	EXPORT static std::shared_ptr<SClass> staticClass();

};

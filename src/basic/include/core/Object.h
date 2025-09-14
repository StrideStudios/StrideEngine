#pragma once

// A simple base class for all objects
// If a derived class does not register itself, it will always be abstract
struct SObject {
	virtual ~SObject() = default;

	virtual const char* getName() const = 0;
};

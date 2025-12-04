#pragma once

/*
 * A simple base class for all objects
 * If a derived class does not register itself, it will always be abstract
 */

struct SClass;

struct SObject {

	virtual ~SObject() = default;

	virtual SClass* getClass() const {
		return staticClass();
	}

	EXPORT static SClass* staticClass();

	// An iterator the parent uses to remove and modify its child
	std::list<SObject*>::iterator itr;

	friend bool operator<(const SObject& fst, const SObject& snd) {
		return true;
	}

	friend bool operator==(const SObject& fst, const SObject& snd) {
		return true;
	}

	friend size_t getHash(const SObject& object) {
		return 0;
	}
};
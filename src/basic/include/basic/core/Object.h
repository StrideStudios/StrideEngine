#pragma once

/*
 * A simple base class for all objects
 * If a derived class does not register itself, it will always be abstract
 */

struct SClass;

template <typename TType>
struct TUnique;

template <typename... TParentClasses>
struct TClass;

struct SObject {

	typedef TClass<SObject> Class;

	virtual ~SObject() = default;

	EXPORT virtual SClass* getClass() const;

	EXPORT static Class* staticClass();

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
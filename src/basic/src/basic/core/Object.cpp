#include "basic/core/Object.h"
#include "basic/core/Class.h"

// Since Object is so generic, it can't implement core/Class.h in its header file
// This is a workaround so class can still be provided

static TUnique<SObject::Class> c{"SObject"};

SClass* SObject::getClass() const {
	return c.staticCast<SClass>();
}

TUnique<SObject::Class>& SObject::staticClass() {
	return c;
}

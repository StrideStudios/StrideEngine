#include "basic/core/Object.h"
#include "basic/core/Class.h"

#include <memory>

// Since Object is so generic, it can't implement core/Class.h in its header file
// This is a workaround so class can still be provided

typedef TClass<SObject> Class;
inline static SClass* c = makeClass<Class>("SObject");

SClass* SObject::staticClass() {
	return c;
}

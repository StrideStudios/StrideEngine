#include "basic/core/Object.h"
#include "basic/core/Class.h"

#include <memory>

// Since Object is so generic, it can't implement core/Class.h in its header file
// This is a workaround so class can still be provided

typedef TClass<SObject> Class;
inline static std::shared_ptr<Class> c = makeClass<Class>("SObject");

std::shared_ptr<SClass> SObject::staticClass() {
	return c;
}

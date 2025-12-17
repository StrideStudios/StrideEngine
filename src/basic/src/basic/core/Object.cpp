#include "basic/core/Object.h"
#include "basic/core/Class.h"

#include <memory>

// Since Object is so generic, it can't implement core/Class.h in its header file
// This is a workaround so class can still be provided

inline static TShared<SObject::Class> c = nullptr;
STATIC_C_BLOCK(
	makeClass(c, "SObject");
)

SClass* SObject::getClass() const {
	return staticClass().get();
}


TShared<SObject::Class> SObject::staticClass() {
	return c;
}

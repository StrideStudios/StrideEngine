#include "ResourceDeallocator.h"
#include "Common.h"

#include "Engine.h"

#define CREATE_SWITCH(inName, inEnum) \
case Type::inEnum: \
vkDestroy##inName(device, *mResource.inName, nullptr); \
break;

void CResourceDeallocator::Resource::destroy() const {
	const VkDevice device = CEngine::get().getDevice().getDevice();
	switch (mType) {
		FOR_EACH_TYPE(CREATE_SWITCH)
		case Type::IMAGE:
			vmaDestroyImage(mResource.image.allocator, *mResource.image.image, mResource.image.allocation);
			break;
		default:
			astNoEntry();
	}
}

#undef CREATE_SWITCH
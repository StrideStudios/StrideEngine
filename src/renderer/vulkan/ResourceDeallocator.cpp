#include "ResourceDeallocator.h"
#include "Engine.h"
#include "ResourceAllocator.h"

#define CREATE_SWITCH(inType, inName, inEnum) \
	case Type::inEnum: \
		vkDestroy##inName(device, mResource.inName, nullptr); \
		break;

void CResourceDeallocator::Resource::destroy() const {
	const VkDevice device = CEngine::device();
	switch (mType) {
		FOR_EACH_TYPE(CREATE_SWITCH)
		case Type::BUFFER:
			CResourceAllocator::deallocateBuffer(mResource.buffer);
			break;
		case Type::IMAGE:
			CResourceAllocator::deallocateImage(mResource.image);
			break;
		default:
			astsNoEntry();
	}
}

#undef CREATE_SWITCH
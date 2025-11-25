#pragma once

#include "rendercore/Renderer.h"
#include "basic/core/Class.h"

// Wrapper around vkb instance that can be destroyed
class CVulkanInstance final : public SObject, public IInitializable, public IDestroyable {

	REGISTER_CLASS(CVulkanInstance, SObject)
    MAKE_LAZY_SINGLETON(CVulkanInstance)

public:

	EXPORT virtual void init() override;

	static const vkb::Instance& instance() { return get().getInstance(); }

	EXPORT static const VkInstance& vkInstance();

	EXPORT const vkb::Instance& getInstance() const;

	EXPORT virtual void destroy() override;

	std::shared_ptr<vkb::Instance> mInstance;
};

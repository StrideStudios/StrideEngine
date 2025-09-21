#pragma once

#include "Renderer.h"
#include "core/Class.h"

// Wrapper around vkb instance that can be destroyed
class CVulkanInstance : public CInstance, public IDestroyable {

	REGISTER_CLASS(CVulkanInstance, CInstance)

public:

	EXPORT CVulkanInstance();

	EXPORT virtual const vkb::Instance& getInstance() const override;

	EXPORT virtual void destroy() override;

	std::shared_ptr<vkb::Instance> mInstance;
};

#pragma once

#include "Engine.h"
#include "Renderer.h"

// Wrapper around vkb instance that can be destroyed
class CVulkanInstance : public CInstance, public IDestroyable {

	REGISTER_CLASS(CVulkanInstance)

public:

	EXPORT CVulkanInstance();

	EXPORT virtual const vkb::Instance& getInstance() const override;

	EXPORT virtual void destroy() override;

	std::unique_ptr<vkb::Instance> mInstance;
};

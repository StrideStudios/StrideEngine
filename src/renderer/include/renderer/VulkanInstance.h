#pragma once

#include "Engine.h"
#include "VkBootstrap.h"

// Wrapper around vkb instance that can be destroyed
struct SVulkanInstance final : IDestroyable {

	SVulkanInstance() {
		vkb::InstanceBuilder builder;

		auto instance = builder.set_app_name(gEngineName)
				.request_validation_layers(true)
				.use_default_debug_messenger()
				.require_api_version(1, 3, 0)
				.build();

		mInstance = instance.value();
	}

	virtual void destroy() override {
		vkb::destroy_instance(mInstance);
	}

	vkb::Instance mInstance;
};

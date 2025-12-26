#include "rendercore/VulkanInstance.h"

#include "VkBootstrap.h"

CVulkanInstance::CVulkanInstance() {
	vkb::InstanceBuilder builder;

	auto instance = builder.set_app_name(gEngineName)
			.request_validation_layers(true)
			.use_default_debug_messenger()
			.require_api_version(1, 3, 0)
			.build();

	//TODO: build creates funcs, but because of linking, the singleton used for funcs is invalid in renderer
	mInstance = std::make_shared<vkb::Instance>(instance.value());
}

const vkb::Instance& CVulkanInstance::getInstance() const {
		return *mInstance;
}

void CVulkanInstance::destroy() {
	vkb::destroy_instance(*mInstance);
}

#include "VulkanDevice.h"

#include <cassert>
#include <iostream>

#include <thread>

#include "Engine.h"

CVulkanDevice::CVulkanDevice() {
    vkb::InstanceBuilder builder;

    auto instance = builder.set_app_name("Stride Engine")
            .request_validation_layers(true)
            .use_default_debug_messenger()
            .require_api_version(1, 3, 0)
            .build();

    m_Instance = std::make_unique<vkb::Instance>(instance.value());
}

void CVulkanDevice::initDevice() {
    // Swapchain Maintenence features
    VkPhysicalDeviceSwapchainMaintenance1FeaturesEXT swapchainMaintenance1Features{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SWAPCHAIN_MAINTENANCE_1_FEATURES_EXT,
        .swapchainMaintenance1 = true
    };

    //TODO: some better way of defining what features are required and what is not

    //vulkan 1.3 features
    VkPhysicalDeviceVulkan13Features features13{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
        .synchronization2 = true,
        .dynamicRendering = true
    };

    //vulkan 1.2 features
    VkPhysicalDeviceVulkan12Features features12{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
        .descriptorIndexing = true,
        .bufferDeviceAddress = true
    };

    //TODO: send out a simple error window telling the user why vulkan crashed (SimpleErrorReporter or something)
    vkb::PhysicalDeviceSelector selector{getInstance()};
    auto physicalDevice = selector
            .set_minimum_version(1, 3)
            .set_required_features_12(features12)
            .set_required_features_13(features13)
            .add_required_extension_features(swapchainMaintenance1Features)
            .set_surface(CEngine::get().getWindow().mVkSurface)
            .select();

    m_PhysicalDevice = std::make_unique<vkb::PhysicalDevice>(physicalDevice.value());

    vkb::DeviceBuilder deviceBuilder{getPhysicalDevice()};
    m_Device = std::make_unique<vkb::Device>(deviceBuilder.build().value());
}

void CVulkanDevice::destroy() const {
    vkb::destroy_device(getDevice());
    vkb::destroy_instance(getInstance());
}

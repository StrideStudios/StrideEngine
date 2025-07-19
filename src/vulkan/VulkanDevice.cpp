#include "VulkanDevice.h"

#include <cassert>
#include <cstdint>
#include <iostream>
#include <vector>

#ifndef GLFW_INCLUDE_VULKAN
    #define GLFW_INCLUDE_VULKAN
#endif
#include "VulkanRenderer.h"
#include "GLFW/glfw3.h"

CVulkanDevice CVulkanDevice::s_Device;

constexpr static int gWidth = 1920;
constexpr static int gHeight = 1080;

CVulkanDevice::CVulkanDevice() {
    vkb::InstanceBuilder builder;

    m_Instance = builder.set_app_name("Stride Engine")
        .request_validation_layers(true)
        .use_default_debug_messenger()
        .require_api_version(1, 3, 0)
        .build()
        .value();

    // Initialize GLFW
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    //GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    m_Window = glfwCreateWindow(gWidth, gHeight, "Stride Engine", nullptr, nullptr);
    assert(m_Window);

    VK_CHECK(glfwCreateWindowSurface(m_Instance, m_Window, nullptr, &m_VkSurface));
    assert(m_VkSurface);

    //TODO: some better way of defining what features are required and what is not

    // Swapchain Maintenence features
    VkPhysicalDeviceSwapchainMaintenance1FeaturesEXT swapchainMaintenance1Features {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SWAPCHAIN_MAINTENANCE_1_FEATURES_EXT,
        .swapchainMaintenance1 = true
    };

    //vulkan 1.3 features
    VkPhysicalDeviceVulkan13Features features13 {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
        .synchronization2 = true,
        .dynamicRendering = true
    };

    //vulkan 1.2 features
    VkPhysicalDeviceVulkan12Features features12 {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
        .descriptorIndexing = true,
        .bufferDeviceAddress = true
    };

    //TODO: send out a simple error window telling the user why vulkan crashed (SimpleErrorReporter or something)
    vkb::PhysicalDeviceSelector selector{m_Instance};
    m_PhysicalDevice = selector
        .set_minimum_version(1, 3)
        .set_required_features_12(features12)
        .set_required_features_13(features13)
        .add_required_extension_features(swapchainMaintenance1Features)
        .set_surface(m_VkSurface)
        .select()
        .value();

    vkb::DeviceBuilder deviceBuilder{m_PhysicalDevice};
    m_Device = deviceBuilder
        .build().value();

    m_Renderer = new CVulkanRenderer(*this);

}

void CVulkanDevice::tick() const {
    // This tells the OS that the application is still responding
    glfwPollEvents();
}


bool CVulkanDevice::windowClosed() const {
    return glfwWindowShouldClose(m_Window);
}

void CVulkanDevice::cleanup() {

    m_Renderer->cleanup();
    delete m_Renderer;

    vkb::destroy_surface(m_Instance, m_VkSurface);
    vkb::destroy_device(m_Device);
    vkb::destroy_instance(m_Instance);

    glfwDestroyWindow(m_Window);
    glfwTerminate();
}

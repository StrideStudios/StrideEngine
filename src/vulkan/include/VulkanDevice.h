#pragma once

// Basic Vulkan Includes
#include <memory>

#include "Common.h"
#include "DeletionQueue.h"
#include "VkBootstrap.h"
#include "VulkanRenderer.h"

class CVulkanDevice {

public:

    CVulkanDevice();

    void initDevice();

    void destroy() const;

    no_discard const vkb::Instance& getInstance() const { return *m_Instance; }

    no_discard const vkb::PhysicalDevice& getPhysicalDevice() const { return *m_PhysicalDevice; }

    no_discard const vkb::Device& getDevice() const { return *m_Device; }

private:

    //
    // Vulkan Rendering
    //


    // Vulkan library handle
    std::unique_ptr<vkb::Instance> m_Instance;

    // GPU chosen as the default device
    std::unique_ptr<vkb::PhysicalDevice> m_PhysicalDevice;

    // Vulkan device for commands
    std::unique_ptr<vkb::Device> m_Device;

};

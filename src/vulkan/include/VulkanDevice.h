#pragma once

// Basic Vulkan Includes
#include "Common.h"
#include "DeletionQueue.h"
#include "VkBootstrap.h"

#define VK_CHECK(call) \
    do { \
        VkResult result_ = call; \
        assert(result_ == VK_SUCCESS); \
    } while (0)

class CVulkanRenderer;
struct GLFWwindow;

class CVulkanDevice {

public:

    CVulkanDevice();

    static CVulkanDevice get() {
        return s_Device;
    }

    void tick() const;

    no_discard bool windowClosed() const;

    void cleanup();

    no_discard CVulkanRenderer* getRenderer() const { return m_Renderer; }

    no_discard vkb::Instance getInstance() const { return m_Instance; }

    no_discard GLFWwindow* getWindow() const { return m_Window; }

    no_discard VkSurfaceKHR getSurface() const { return m_VkSurface; }

    no_discard const vkb::PhysicalDevice& getPhysicalDevice() const { return m_PhysicalDevice; }

    no_discard const vkb::Device& getDevice() const { return m_Device; }

private:

    // The Current Device
    static CVulkanDevice s_Device;

    CVulkanRenderer* m_Renderer;

    // Active game window
    GLFWwindow* m_Window;

    // Vulkan library handle
    vkb::Instance m_Instance;

    // Vulkan window surface
    VkSurfaceKHR m_VkSurface;

    // GPU chosen as the default device
    vkb::PhysicalDevice m_PhysicalDevice;

    // Vulkan device for commands
    vkb::Device m_Device;

};

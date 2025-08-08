#pragma once

// Basic Vulkan Includes
#include <memory>
#include <map>

#include "Common.h"
#include "VkBootstrap.h"

enum class EQueueType : uint8 {
    GRAPHICS,
    // Upload commands may be on different threads, so it should use a separate queue so it doesn't lag the renderer
    UPLOAD,
    MAX
};

struct SQueue {
    VkQueue mQueue;
    uint32 mFamily;
};

// Define how to create queues
static std::map<vkb::QueueType, std::map<EQueueType, float>> queueFamilies {
    {
        vkb::QueueType::graphics,
        {
            {EQueueType::GRAPHICS, 1.f},
            {EQueueType::UPLOAD, 0.f}
        }
    },
};

class CVulkanDevice {

public:

    CVulkanDevice();

    static SQueue getQueue(EQueueType inType);

    void initDevice();

    void destroy() const;

    no_discard constexpr const vkb::Instance& getInstance() const { return *m_Instance; }

    no_discard constexpr const vkb::PhysicalDevice& getPhysicalDevice() const { return *m_PhysicalDevice; }

    no_discard constexpr const vkb::Device& getDevice() const { return *m_Device; }

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

    // Queues are created by the driver, so it is only natural they are stored in the device
	std::map<EQueueType, SQueue> mQueues;

};

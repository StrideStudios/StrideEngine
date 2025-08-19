#pragma once

#include <memory>
#include <map>

#include "Renderer.h"
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

class CVulkanDevice : public CDevice, public IInitializable<>, public IDestroyable {

public:

    static SQueue getQueue(EQueueType inType);

    virtual void init() override;

    virtual void destroy() override;

    no_discard virtual const vkb::PhysicalDevice& getPhysicalDevice() const override { return *m_PhysicalDevice; }

    no_discard virtual const vkb::Device& getDevice() const override { return *m_Device; }

private:

    //
    // Vulkan Rendering
    //

    // GPU chosen as the default device
    std::unique_ptr<vkb::PhysicalDevice> m_PhysicalDevice;

    // Vulkan device for commands
    std::unique_ptr<vkb::Device> m_Device;

    // Queues are created by the driver, so it is only natural they are stored in the device
	std::map<EQueueType, SQueue> mQueues;

};

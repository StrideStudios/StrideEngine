#pragma once

#include <memory>
#include <map>
#include <vulkan/vulkan_core.h>

#include "Renderer.h"

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

class CVulkanDevice : public CDevice, public TInitializable<class CVulkanInstance*, VkSurfaceKHR>, public IDestroyable {

    REGISTER_CLASS(CVulkanDevice)

public:

    EXPORT static SQueue getQueue(EQueueType inType);

    EXPORT virtual void init(CVulkanInstance* inInstance, VkSurfaceKHR inSurface) override;

    EXPORT virtual void destroy() override;

    no_discard EXPORT virtual const vkb::PhysicalDevice& getPhysicalDevice() const override;

    no_discard EXPORT virtual const vkb::Device& getDevice() const override;

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

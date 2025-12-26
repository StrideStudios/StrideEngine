#pragma once

#include <memory>
#include <map>
#include <vulkan/vulkan_core.h>

#include "rendercore/Renderer.h"
#include "basic/core/Class.h"

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

class CVulkanDevice final : public SObject, public IDestroyable {

	REGISTER_CLASS(CVulkanDevice, SObject)

public:

    CVulkanDevice() = default;
    EXPORT CVulkanDevice(TShared<CVulkanInstance> inInstance, VkSurfaceKHR inSurface);

    EXPORT SQueue getQueue(EQueueType inType);

    EXPORT virtual void destroy() override;

    no_discard EXPORT const vkb::PhysicalDevice& getPhysicalDevice() const;

    no_discard EXPORT const vkb::Device& getDevice() const;

private:

    //
    // Vulkan Rendering
    //

    // GPU chosen as the default device
    std::shared_ptr<vkb::PhysicalDevice> m_PhysicalDevice;

    // Vulkan device for commands
    std::shared_ptr<vkb::Device> m_Device;

    // Queues are created by the driver, so it is only natural they are stored in the device
	std::map<EQueueType, SQueue> mQueues;

};

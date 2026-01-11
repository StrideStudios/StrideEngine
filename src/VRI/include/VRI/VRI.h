#pragma once

#include "basic/core/Common.h"
#include "sstl/Memory.h"
#include "sstl/PriorityMap.h"

namespace vkb {
	struct Instance;
    struct Device;
    struct PhysicalDevice;
}

enum class EQueueType : uint8 {
    GRAPHICS,
    // Upload commands may be on different threads, so it should use a separate queue so it doesn't lag the renderer
    UPLOAD,
    MAX
};

struct SQueue {
    struct VkQueue_T* mQueue;
    uint32 mFamily;
};

struct VmaAllocator_T;
struct VkSurfaceKHR_T;

// A class defining the interface vulkan uses for all of its calls and resource management
class CVRI {

public:

    EXPORT static TFrail<CVRI> get();

    EXPORT void init(struct SDL_Window* inWindow);

    EXPORT void destroy() const;

    TFrail<vkb::Instance> getInstance() const { return m_Instance; }

    TFrail<VkSurfaceKHR_T> getSurface() const { return m_Surface; }

    TFrail<vkb::Device> getDevice() const { return m_Device; }

    TFrail<VmaAllocator_T> getAllocator() const { return m_Allocator; }

    SQueue& getQueue(const EQueueType inType) {
        return mQueues.get(inType);
    }

private:

	TUnique<vkb::Instance> m_Instance = nullptr;

	VkSurfaceKHR_T* m_Surface = nullptr;

    TUnique<vkb::Device> m_Device = nullptr;

    VmaAllocator_T* m_Allocator = nullptr;

    TPriorityMap<EQueueType, SQueue> mQueues;

};
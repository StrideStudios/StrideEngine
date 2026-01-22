#pragma once

#include "VRISwapchain.h"
#include "sstl/Array.h"
#include "sstl/Memory.h"
#include "sstl/Queue.h"
#include "VRI/VRIResources.h"

struct VmaAllocator_T;

class CVRIAllocator {

public:

    EXPORT CVRIAllocator();

    TFrail<VmaAllocator_T> get() const { return m_Allocator; }

    EXPORT void destroy2();

    void pushResource(const TFrail<SVRIResource>& inResource) {
        m_Resources.push(inResource);
    }

    template <typename TResource, typename... TArgs>
    TShared<TResource> allocateResource(TArgs&&... args) {
        auto ptr = TShared<TResource>{std::forward<TArgs>(args)...};
        m_Resources.push(ptr.template staticCast<SVRIResource>());
        return ptr;
    }

    // Tells the resource to destroy itself, removes from tracking
    void releaseResource(const TFrail<SVRIResource>& inResource) {
        if (m_Destroyed) return; // The allocator, upon destruction, automatically destroys all assets
        if (m_Resources.contains(inResource.get())) {
            const auto index = m_Resources.find(inResource.get());
            m_DeletionQueue.get(CVRI::get()->getSwapchain()->m_Buffering.getFrameIndex()).push(m_Resources.get(index)->getDestroyer());
            m_Resources.pop(index);
        }
    }

    void immediateRelease(const TFrail<SVRIResource>& inResource) {
        if (m_Destroyed) return; // The allocator, upon destruction, automatically destroys all assets
        if (m_Resources.contains(inResource.get())) {
            m_Resources.pop(inResource);
            inResource->getDestroyer()();
        }
    }

    void popDeferredQueue(const size_t inFrameIndex) {
        m_DeletionQueue.get(inFrameIndex).forEach([](size_t, const std::function<void()>& inDestroyer) {
            inDestroyer();
        });
        m_DeletionQueue.get(inFrameIndex).clear();
    }

private:

    bool m_Destroyed = false;

    VmaAllocator_T* m_Allocator = nullptr;

    // Although we own the resource, we can use weak pointers to track it
    TVector<TFrail<SVRIResource>> m_Resources;

    TArray<TQueue<std::function<void()>>, 2> m_DeletionQueue;
};
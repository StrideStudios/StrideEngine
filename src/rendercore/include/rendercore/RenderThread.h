#pragma once

#include <functional>

#include "Renderer.h"
#include "basic/core/Threading.h"
#include "sstl/Memory.h"
#include "sstl/Vector.h"

#include "tracy/Tracy.hpp"

class CRenderThread : public CPersistentThread {

public:

    struct Context : SRendererInfo {

        template <typename TRendererType>
        requires std::is_base_of_v<CObjectRenderer, TRendererType>
        void addRendererProxy(const TRendererType::Proxy& inProxy) {

        }
    };

    //TODO: fill ctx
    CRenderThread(): CPersistentThread([this] {
        TracyCSetThreadName("Render Thread")
        Context ctx;
        while (true) {
            TracyCZoneNC(zone, "Render Thread", 0xff0000, 1);
            {
                ZoneScopedN("Render");
                ctx.renderer->render(ctx);
            }
            {
                ZoneScopedN("Commands");
                while (mRendererTaskQueue.getSize() > 0) {
                    mRendererTaskQueue.top()(ctx);
                    mRendererTaskQueue.pop();
                }
            }
            TracyCZoneEnd(zone);
        }
    }) {}

    void enqueue(const std::function<void(Context&)>& func) {
        mRendererTaskQueue.push(func);
    }

private:

    TVector<std::function<void(Context&)>> mRendererTaskQueue;
};

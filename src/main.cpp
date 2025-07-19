#include "VulkanRenderer.h"
#include "vulkan/include/VulkanDevice.h"

int main() {
    CVulkanDevice device = CVulkanDevice::get();

    bool engineExit = false;
    while (!engineExit) {
        device.tick();
        device.getRenderer()->draw();
        if (device.windowClosed()) {
            engineExit = true;
        }
    }

    device.cleanup();
}
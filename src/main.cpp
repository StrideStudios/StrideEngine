#include <iostream>
#include "Engine.h"

// These includes prevent CEngine from throwing errors
#include "VulkanDevice.h"
#include "VulkanRenderer.h"

int main() {
    CEngine::get().run();
    return 0;
}
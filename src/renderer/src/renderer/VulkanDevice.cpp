#include "renderer/VulkanDevice.h"

#include "VkBootstrap.h"
#include "renderer/VulkanInstance.h"
#include "renderer/VulkanRenderer.h"

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

SQueue CVulkanDevice::getQueue(const EQueueType inType) {
    return CVulkanRenderer::get()->m_Device->mQueues[inType];
}

void CVulkanDevice::init(CVulkanInstance* inInstance, VkSurfaceKHR inSurface) {
    // Swapchain Maintenance features
    VkPhysicalDeviceSwapchainMaintenance1FeaturesEXT swapchainMaintenance1Features{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SWAPCHAIN_MAINTENANCE_1_FEATURES_EXT,
        .swapchainMaintenance1 = false
    };

    //TODO: some better way of defining what features are required and what is not

    // physical device features
    VkPhysicalDeviceFeatures features{
        .fillModeNonSolid = true,
        .wideLines = true,
        .textureCompressionBC = true,
        .shaderInt64 = true
    };

    //vulkan 1.3 features
    VkPhysicalDeviceVulkan13Features features13{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
        .shaderDemoteToHelperInvocation = true,
        .synchronization2 = true,
        .dynamicRendering = true
    };

    //vulkan 1.2 features
    VkPhysicalDeviceVulkan12Features features12{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
        .descriptorIndexing = true,
        .shaderSampledImageArrayNonUniformIndexing = true,
        .descriptorBindingUniformBufferUpdateAfterBind = true, //TODO: not sure al these features are necessary
        .descriptorBindingSampledImageUpdateAfterBind = true, //descriptorBindingUpdateUnusedWhilePending
        .descriptorBindingStorageImageUpdateAfterBind = true,
        .descriptorBindingStorageBufferUpdateAfterBind = true,
        .descriptorBindingUniformTexelBufferUpdateAfterBind = true,
        .descriptorBindingStorageTexelBufferUpdateAfterBind = true,
        .descriptorBindingPartiallyBound = true,
        .descriptorBindingVariableDescriptorCount = true,
        .runtimeDescriptorArray = true,
        .bufferDeviceAddress = true
    };

    //TODO: send out a simple error window telling the user why vulkan crashed (SimpleErrorReporter or something)
    vkb::PhysicalDeviceSelector selector{inInstance->getInstance()};
    auto physicalDevice = selector
            .set_minimum_version(1, 3)
            .set_required_features(features)
            .set_required_features_12(features12)
            .set_required_features_13(features13)
            .add_required_extension_features(swapchainMaintenance1Features)
            .set_surface(inSurface)
            .select();

    m_PhysicalDevice = std::make_shared<vkb::PhysicalDevice>(physicalDevice.value());

    std::map<vkb::QueueType, size_t> queueBits;

    //TODO: present queue
    queueBits.emplace(vkb::QueueType::graphics, VK_QUEUE_GRAPHICS_BIT);
    queueBits.emplace(vkb::QueueType::compute, VK_QUEUE_COMPUTE_BIT | ~VK_QUEUE_TRANSFER_BIT);
    queueBits.emplace(vkb::QueueType::transfer, VK_QUEUE_TRANSFER_BIT | ~VK_QUEUE_COMPUTE_BIT);

    // Create queueDescriptions based off queueFamilies input
    std::vector<vkb::CustomQueueDescription> queueDescriptions;
    auto families = m_PhysicalDevice->get_queue_families();
    for (size_t i = 0; i < families.size(); i++) {
        for (const auto& [queueType, map] : queueFamilies) {
            if (families[i].queueFlags & queueBits[queueType]) {
                std::vector<float> vector;
                for (const auto& values : map) {
                    vector.push_back(values.second);
                }
                queueDescriptions.emplace_back(static_cast<uint32>(i), vector);
            }
        }
    }

    vkb::DeviceBuilder deviceBuilder{*m_PhysicalDevice};
    deviceBuilder.custom_queue_setup(queueDescriptions);
    m_Device = std::make_shared<vkb::Device>(deviceBuilder.build().value());

    // Get queues from the device
    for (const auto&[type, map] : queueFamilies) {
        uint32 family = m_Device->get_queue_index(type).value();
        int32 index = 0;
        for (const auto& queueType : map) {
            VkQueue queue; vkGetDeviceQueue(*m_Device, family, index, &queue);
            index++;
            SQueue inQueue {
                .mQueue = queue,
                .mFamily = family
            };
            mQueues.emplace(queueType.first, inQueue);
        }
    }
}

void CVulkanDevice::destroy() {
    vkb::destroy_device(*m_Device);
}

const vkb::PhysicalDevice& CVulkanDevice::getPhysicalDevice() const {
    return *m_PhysicalDevice;
}

const vkb::Device& CVulkanDevice::getDevice() const {
    return *m_Device;
}

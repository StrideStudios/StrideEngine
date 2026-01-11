#include "VRI/VRI.h"

#include <vma/vk_mem_alloc.h>

#include "VkBootstrap.h"
#include "SDL3/SDL_vulkan.h"
#include "VRI/VRIResources.h"

TFrail<CVRI> CVRI::get() {
    static TUnique<CVRI> vri;
    return vri;
}

void CVRI::init(SDL_Window* inWindow) {
	// Ensure the VRI is only initialized once
	astsOnce(CVRI);

	// Initializes the vkb instance
	vkb::InstanceBuilder builder;

	auto instance = builder.set_app_name(gEngineName)
			.request_validation_layers(true)
			.use_default_debug_messenger()
			.require_api_version(1, 3, 0)
			.build();

	m_Instance = TUnique{instance.value()};

	// Create a surface for Device to reference
	SDL_Vulkan_CreateSurface(inWindow, m_Instance->instance, nullptr, &m_Surface);

	// Create the vulkan device
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
    vkb::PhysicalDeviceSelector selector{*m_Instance.get()};
    auto physicalDevice = selector
            .set_minimum_version(1, 3)
            .set_required_features(features)
            .set_required_features_12(features12)
            .set_required_features_13(features13)
            .add_required_extension_features(swapchainMaintenance1Features)
            .set_surface(m_Surface)
            .select();

    TPriorityMap<vkb::QueueType, size_t> queueBits;

    //TODO: present queue
    queueBits.push(vkb::QueueType::graphics, VK_QUEUE_GRAPHICS_BIT);
    queueBits.push(vkb::QueueType::compute, VK_QUEUE_COMPUTE_BIT | ~VK_QUEUE_TRANSFER_BIT);
    queueBits.push(vkb::QueueType::transfer, VK_QUEUE_TRANSFER_BIT | ~VK_QUEUE_COMPUTE_BIT);

    // Define how to create queues
    const static TPriorityMap queueFamilies {
        TPair{
            vkb::QueueType::graphics,
            TPriorityMap{
                TPair{EQueueType::GRAPHICS, 1.f},
                TPair{EQueueType::UPLOAD, 0.f}
            }
        },
    };

    // Create queueDescriptions based off queueFamilies input
    std::vector<vkb::CustomQueueDescription> queueDescriptions;
    auto families = physicalDevice.value().get_queue_families();

    for (size_t i = 0; i < families.size(); i++) {
        queueFamilies.forEach([&](auto pair) {
            if (families[i].queueFlags & queueBits.get(pair.first())) {
                std::vector<float> vector;
                pair.second().forEach([&](auto pair2) {
                    vector.push_back(pair2.second());
                });
                queueDescriptions.emplace_back(static_cast<uint32>(i), vector);
            }
        });
    }

    vkb::DeviceBuilder deviceBuilder{physicalDevice.value()};
    deviceBuilder.custom_queue_setup(queueDescriptions);
    m_Device = TUnique{deviceBuilder.build().value()};

    // Get queues from the device
    queueFamilies.forEach([&](auto pair) {
        const uint32 family = m_Device->get_queue_index(pair.first()).value();
        int32 index = 0;
        pair.second().forEach([&](auto pair2) {
            VkQueue queue;
            vkGetDeviceQueue(*m_Device, family, index, &queue);
            index++;
            const SQueue inQueue {
                .mQueue = queue,
                .mFamily = family
            };
            mQueues.push(pair2.first(), inQueue);
        });
    });

    const VmaAllocatorCreateInfo allocatorInfo {
        .flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT,
        .physicalDevice = m_Device->physical_device,
        .device = m_Device->device,
        .instance = m_Instance->instance
    };

    VK_CHECK(vmaCreateAllocator(&allocatorInfo, &m_Allocator));

}

void CVRI::destroy() const {
    vmaDestroyAllocator(m_Allocator);
    vkb::destroy_device(*m_Device);
    vkb::destroy_instance(*m_Instance);
}
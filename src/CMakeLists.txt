
# Make sure files can pull from include paths
include_directories("core/include")
include_directories("renderer/include")
include_directories("engine/include")
include_directories("vulkan/include")

add_executable(StrideEngine
        core/include/DeletionQueue.h
        core/include/BasicTypes.h
        core/include/Common.h

        engine/Engine.cpp
        engine/include/Engine.h

        renderer/Swapchain.cpp
        renderer/include/Swapchain.h
        renderer/EngineTextures.cpp
        renderer/include/EngineTextures.h

        vulkan/include/VulkanDevice.h
        vulkan/VulkanDevice.cpp
        vulkan/VulkanRenderer.cpp
        vulkan/include/VulkanRenderer.h
        vulkan/include/VulkanUtils.h

        main.cpp

)

find_package(Vulkan REQUIRED)
target_link_libraries(StrideEngine PUBLIC
        fmt::fmt
        SDL3::SDL3
        Vulkan::Vulkan
        vk-bootstrap::vk-bootstrap
        GPUOpen::VulkanMemoryAllocator
)

# Compile Shaders
include(CompileShaders)
compile_shaders(StrideEngine
        gradient.comp
)
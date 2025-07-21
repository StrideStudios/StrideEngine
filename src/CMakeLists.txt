
# Make sure files can pull from include paths
include_directories("core/include")
include_directories("renderer/include")
include_directories("engine/include")
include_directories("vulkan/include")
include_directories("imgui/include")

add_executable(StrideEngine
        core/include/DeletionQueue.h
        core/include/BasicTypes.h
        core/include/Common.h

        engine/include/Engine.h
        engine/Engine.cpp

        renderer/include/Swapchain.h
        renderer/include/EngineTextures.h
        renderer/Swapchain.cpp
        renderer/EngineTextures.cpp

        vulkan/include/VulkanDevice.h
        vulkan/include/VulkanRenderer.h
        vulkan/include/VulkanUtils.h
        vulkan/VulkanDevice.cpp
        vulkan/VulkanRenderer.cpp

        imgui/include/imconfig.h
        imgui/include/imgui.h
        imgui/include/imgui_internal.h
        imgui/include/imstb_rectpack.h
        imgui/include/imstb_textedit.h
        imgui/include/imstb_truetype.h
        imgui/include/imgui_impl_vulkan.h
        imgui/include/imgui_impl_sdl3.h
        imgui/imgui.cpp
        imgui/imgui_demo.cpp
        imgui/imgui_draw.cpp
        imgui/imgui_tables.cpp
        imgui/imgui_widgets.cpp
        imgui/imgui_impl_vulkan.cpp
        imgui/imgui_impl_sdl3.cpp

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
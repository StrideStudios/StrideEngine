
# Make sure files can pull from include paths
include_directories("core/include")
include_directories("renderer/include")
include_directories("engine/include")
include_directories("imgui/include")

add_executable(StrideEngine

        # Core

        core/include/DeletionQueue.h
        core/include/BasicTypes.h
        core/include/Common.h

        # Engine

        engine/include/EngineSettings.h
        engine/EngineSettings.cpp
        engine/include/Engine.h
        engine/Engine.cpp

        # Renderer

        renderer/include/ResourceDeallocator.h
        renderer/include/ShaderCompiler.h
        renderer/ShaderCompiler.cpp
        renderer/include/Swapchain.h
        renderer/include/EngineTextures.h
        renderer/include/VulkanDevice.h
        renderer/include/VulkanRenderer.h
        renderer/include/VulkanUtils.h
        renderer/Swapchain.cpp
        renderer/EngineTextures.cpp
        renderer/VulkanDevice.cpp
        renderer/VulkanRenderer.cpp

        # Dear ImGui

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
        renderer/ResourceDeallocator.cpp
)

find_package(Vulkan REQUIRED)
target_link_libraries(StrideEngine PUBLIC
        fmt::fmt
        SDL3::SDL3
        Vulkan::Vulkan
        vk-bootstrap::vk-bootstrap
        GPUOpen::VulkanMemoryAllocator
        glslang::glslang-default-resource-limits
        glslang::glslang
)
# Tell cmake to copy dlls to the target directory
add_custom_command(TARGET StrideEngine POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_RUNTIME_DLLS:StrideEngine> $<TARGET_FILE_DIR:StrideEngine>
        COMMAND_EXPAND_LISTS
)

file(COPY "${PROJECT_SOURCE_DIR}/shaders/" DESTINATION "${PROJECT_BINARY_DIR}/shaders")

# Compile Shaders
#include(CompileShaders)
#compile_shaders(StrideEngine
#        basic/gradient.comp
#)
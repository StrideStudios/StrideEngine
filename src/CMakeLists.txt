
# Make sure files can pull from include paths
include_directories("core/include")
include_directories("renderer/include")
include_directories("renderer/vulkan/include")
include_directories("engine/include")
include_directories("imgui/include")
include_directories("stb/include")

add_executable(StrideEngine

        # Core

        core/include/Hashing.h
        core/include/BasicTypes.h
        core/include/Common.h

        # Engine

        engine/include/EngineSettings.h
        engine/EngineSettings.cpp
        engine/include/StaticMesh.h
        engine/include/MeshLoader.h
        engine/MeshLoader.cpp
        engine/include/Engine.h
        engine/Engine.cpp
        engine/include/Material.h
        engine/include/Camera.h
        engine/Camera.cpp

        # Renderer

        renderer/vulkan/include/VulkanUtils.h
        renderer/vulkan/VulkanUtils.cpp
        renderer/vulkan/include/Swapchain.h
        renderer/vulkan/Swapchain.cpp
        renderer/vulkan/include/VulkanDevice.h
        renderer/vulkan/VulkanDevice.cpp
        renderer/vulkan/include/ShaderCompiler.h
        renderer/vulkan/ShaderCompiler.cpp
        renderer/vulkan/include/VulkanRenderer.h
        renderer/vulkan/VulkanRenderer.cpp
        renderer/include/ResourceManager.h
        renderer/ResourceManager.cpp
        renderer/include/EngineTextures.h
        renderer/EngineTextures.cpp
        renderer/include/PipelineBuilder.h
        renderer/PipelineBuilder.cpp
        renderer/include/EngineBuffers.h
        renderer/EngineBuffers.cpp
        renderer/include/GpuScene.h
        renderer/GpuScene.cpp
        renderer/include/SceneViewport.h
        renderer/include/DescriptorManager.h
        renderer/DescriptorManager.cpp

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

        # stb
        stb/include/stb_image.h
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
        fastgltf::fastgltf
        Tracy::TracyClient
)
target_compile_definitions(TracyClient PRIVATE TRACY_ENABLE>)
target_compile_definitions(StrideEngine PRIVATE TRACY_ENABLE>)
# Tell cmake to copy dlls to the target directory
add_custom_command(TARGET StrideEngine POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_RUNTIME_DLLS:StrideEngine> $<TARGET_FILE_DIR:StrideEngine>
        COMMAND_EXPAND_LISTS
)
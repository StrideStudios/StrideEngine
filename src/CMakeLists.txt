
# Make sure files can pull from include paths
include_directories("core/include")
include_directories("renderer/include")
include_directories("renderer/vulkan/include")
include_directories("renderer/passes/include")
include_directories("engine/include")
include_directories("imgui/include")
include_directories("../third_party/basis_universal")

add_executable(StrideEngine

        # Core

        core/include/ResourceManager.h
        core/include/Hashing.h
        core/include/Common.h
        core/include/Archive.h
        core/include/Threading.h
        core/include/Paths.h

        # Engine

        engine/include/Scene.h
        engine/include/EngineSettings.h
        engine/EngineSettings.cpp
        engine/include/StaticMesh.h
        engine/include/Viewport.h
        engine/include/Engine.h
        engine/Engine.cpp
        engine/include/Material.h
        engine/Material.cpp
        engine/include/Camera.h
        engine/Camera.cpp
        engine/include/SceneObject.h
        engine/include/Sprite.h
        engine/include/Input.h
        engine/Input.cpp
        engine/Viewport.cpp
        engine/include/EngineUI.h
        engine/include/EngineLoader.h
        engine/EngineLoader.cpp

        # Renderer

        renderer/include/VulkanResourceManager.h
        renderer/VulkanResourceManager.cpp
        renderer/include/EngineTextures.h
        renderer/EngineTextures.cpp
        renderer/include/PipelineBuilder.h
        renderer/PipelineBuilder.cpp
        renderer/include/TestRenderer.h
        renderer/TestRenderer.cpp

        # Vulkan

        renderer/vulkan/include/VulkanUtils.h
        renderer/vulkan/VulkanUtils.cpp
        renderer/vulkan/include/Swapchain.h
        renderer/vulkan/Swapchain.cpp
        renderer/vulkan/include/VulkanInstance.h
        renderer/vulkan/include/VulkanDevice.h
        renderer/vulkan/VulkanDevice.cpp
        renderer/vulkan/include/ShaderCompiler.h
        renderer/vulkan/ShaderCompiler.cpp
        renderer/vulkan/include/VulkanRenderer.h
        renderer/vulkan/VulkanRenderer.cpp

        # Passes

        renderer/passes/include/Pass.h
        renderer/passes/Pass.cpp
        renderer/passes/include/MeshPass.h
        renderer/passes/MeshPass.cpp
        renderer/passes/include/SpritePass.h
        renderer/passes/SpritePass.cpp

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
        imgui/include/imgui_stdlib.h
        imgui/imgui_stdlib.cpp
)

find_package(Vulkan REQUIRED)
target_link_libraries(StrideEngine PUBLIC
        fmt::fmt
        SDL3::SDL3
        Vulkan::Vulkan
        vk-bootstrap::vk-bootstrap
        glslang::glslang-default-resource-limits
        glslang::glslang
        fastgltf::fastgltf
        Tracy::TracyClient
        meshoptimizer
        basisu_encoder
)
target_compile_definitions(TracyClient PRIVATE TRACY_ENABLE>)
target_compile_definitions(StrideEngine PRIVATE TRACY_ENABLE>)
# Tell cmake to copy dlls to the target directory
add_custom_command(TARGET StrideEngine POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_RUNTIME_DLLS:StrideEngine> $<TARGET_FILE_DIR:StrideEngine>
        COMMAND_EXPAND_LISTS
)
add_library(StrideEngine-renderer SHARED
        include/renderer/VulkanResourceManager.h
        src/renderer/VulkanResourceManager.cpp
        include/renderer/EngineTextures.h
        src/renderer/EngineTextures.cpp
        include/renderer/Swapchain.h
        src/renderer/Swapchain.cpp
        include/renderer/VulkanInstance.h
        include/renderer/VulkanDevice.h
        src/renderer/VulkanDevice.cpp
        include/renderer/VulkanRenderer.h
        src/renderer/VulkanRenderer.cpp
        include/renderer/EngineLoader.h
        src/renderer/EngineLoader.cpp
        include/renderer/EngineUI.h
        src/renderer/EngineUI.cpp

        include/passes/Pass.h
        src/passes/Pass.cpp
        include/passes/MeshPass.h
        src/passes/MeshPass.cpp
        include/passes/SpritePass.h
        src/passes/SpritePass.cpp
        include/passes/EngineUIPass.h
        src/passes/EngineUIPass.cpp
)

set_target_properties(StrideEngine-renderer PROPERTIES LINKER_LANGUAGE CXX)

target_include_directories(StrideEngine-renderer PUBLIC
        "include"
        "../../third_party/basis_universal"
        "../basic/include"
        "../rendercore/include"
        "../engine/include"
        "../scene/include"
        "../imgui/include"
)

target_link_libraries(StrideEngine-renderer PUBLIC
        glslang::glslang-default-resource-limits
        glslang::glslang
        fastgltf::fastgltf
        meshoptimizer
        basisu_encoder

        StrideEngine-basic
        StrideEngine-rendercore
        StrideEngine-engine
        StrideEngine-scene
        StrideEngine-imgui
)
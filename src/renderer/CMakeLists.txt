add_library(StrideEngine-renderer SHARED
        core/include/VulkanResourceManager.h
        core/src/VulkanResourceManager.cpp
        core/include/EngineTextures.h
        core/src/EngineTextures.cpp
        core/include/TestRenderer.h
        core/src/TestRenderer.cpp
        core/include/VulkanUtils.h
        core/src/VulkanUtils.cpp
        core/include/Swapchain.h
        core/src/Swapchain.cpp
        core/include/VulkanInstance.h
        core/include/VulkanDevice.h
        core/src/VulkanDevice.cpp
        core/include/VulkanRenderer.h
        core/src/VulkanRenderer.cpp
        core/include/Material.h
        core/src/Material.cpp
        core/include/StaticMesh.h
        core/include/EngineLoader.h
        core/src/EngineLoader.cpp
        core/include/EngineUI.h
        core/src/EngineUI.cpp

        passes/include/Pass.h
        passes/src/Pass.cpp
        passes/include/MeshPass.h
        passes/src/MeshPass.cpp
        passes/include/SpritePass.h
        passes/src/SpritePass.cpp
        passes/include/EngineUIPass.h
        passes/src/EngineUIPass.cpp
)

set_target_properties(StrideEngine-renderer PROPERTIES LINKER_LANGUAGE CXX)

target_include_directories(StrideEngine-renderer PUBLIC
        "core/include"
        "passes/include"
        "../../third_party/basis_universal"
        "../core/include"
        "../control/include"
        "../engine/include"
        "../scene/include"
        "../imgui/include"
)

find_package(Vulkan REQUIRED)
target_link_libraries(StrideEngine-renderer PUBLIC
        glm::glm
        Vulkan::Vulkan
        glslang::glslang-default-resource-limits
        glslang::glslang
        fastgltf::fastgltf
        meshoptimizer
        basisu_encoder

        StrideEngine-core
        StrideEngine-control
        StrideEngine-engine
        StrideEngine-scene
        StrideEngine-imgui
)
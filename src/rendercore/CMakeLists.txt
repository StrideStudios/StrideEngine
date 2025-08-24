add_library(StrideEngine-rendercore SHARED
        include/VulkanResourceManager.h
        src/VulkanResourceManager.cpp
        include/Material.h
        include/StaticMesh.h
        include/Renderer.h
        src/Renderer.cpp
        include/VulkanResources.h
        src/VulkanResources.cpp
        include/VulkanUtils.h
        src/VulkanUtils.cpp
        include/Pass.h
        src/Pass.cpp
        include/Instancer.h
        src/Instancer.cpp
)

set_target_properties(StrideEngine-rendercore PROPERTIES LINKER_LANGUAGE CXX)

target_include_directories(StrideEngine-rendercore PUBLIC
        "include"
        "../basic/include"
        "../imgui/include"
)

find_package(Vulkan REQUIRED)
target_link_libraries(StrideEngine-rendercore PUBLIC
        glslang::glslang-default-resource-limits #TODO: dont want glslang in rendercore but fine for now
        glslang::glslang

        Vulkan::Vulkan
        vk-bootstrap::vk-bootstrap

        StrideEngine-basic
        StrideEngine-imgui
)
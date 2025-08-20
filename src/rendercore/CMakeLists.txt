add_library(StrideEngine-rendercore SHARED
        include/Material.h
        include/StaticMesh.h
        include/Renderer.h
        src/Renderer.cpp
        include/VulkanResources.h
        src/VulkanResources.cpp
        include/VulkanUtils.h
        src/VulkanUtils.cpp
)

set_target_properties(StrideEngine-rendercore PROPERTIES LINKER_LANGUAGE CXX)

target_include_directories(StrideEngine-rendercore PUBLIC
        "include"
        "../basic/include"
        "../imgui/include"
)

find_package(Vulkan REQUIRED)
target_link_libraries(StrideEngine-rendercore PUBLIC
        Vulkan::Vulkan
        vk-bootstrap::vk-bootstrap

        StrideEngine-basic
        StrideEngine-imgui
)
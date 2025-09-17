create_module(StrideEngine-rendercore SHARED)

find_package(Vulkan REQUIRED)
add_dependencies(StrideEngine-rendercore
    PRIVATE
        vk-bootstrap::vk-bootstrap

        glslang::glslang-default-resource-limits
        glslang::glslang
    PUBLIC
        Vulkan::Vulkan

        StrideEngine-basic
)
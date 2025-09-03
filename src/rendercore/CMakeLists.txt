create_module(StrideEngine-rendercore SHARED)

find_package(Vulkan REQUIRED)
add_dependencies(StrideEngine-rendercore
    PUBLIC
        glslang::glslang-default-resource-limits #TODO: dont want glslang in rendercore but fine for now
        glslang::glslang

        Vulkan::Vulkan
        vk-bootstrap::vk-bootstrap

        StrideEngine-basic
        StrideEngine-imgui
)
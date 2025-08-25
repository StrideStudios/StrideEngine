find_package(Vulkan REQUIRED)
create_module(StrideEngine-rendercore PUBLIC SHARED
        glslang::glslang-default-resource-limits #TODO: dont want glslang in rendercore but fine for now
        glslang::glslang

        Vulkan::Vulkan
        vk-bootstrap::vk-bootstrap

        StrideEngine-basic
        StrideEngine-imgui
)
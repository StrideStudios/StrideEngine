create_module(StrideEngine-VRI SHARED)

find_package(Vulkan REQUIRED COMPONENTS dxc)
add_dependencies(StrideEngine-VRI
    PRIVATE
        SDL3::SDL3
        vk-bootstrap::vk-bootstrap
        Vulkan::Vulkan
        Vulkan::dxc_lib

    PUBLIC

        StrideEngine-basic
)
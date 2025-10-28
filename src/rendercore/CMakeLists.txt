create_module(StrideEngine-rendercore SHARED)

find_package(Vulkan REQUIRED COMPONENTS dxc)
add_dependencies(StrideEngine-rendercore
    PRIVATE
        vk-bootstrap::vk-bootstrap

        Vulkan::dxc_lib
    PUBLIC
        Vulkan::Vulkan

        StrideEngine-basic
)
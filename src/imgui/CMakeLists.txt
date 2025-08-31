create_module(StrideEngine-imgui SHARED)

find_package(Vulkan REQUIRED)
add_public_dependencies(StrideEngine-imgui
        SDL3::SDL3
        Vulkan::Vulkan
)
create_module(StrideEngine-imgui SHARED)

find_package(Vulkan REQUIRED)
add_dependencies(StrideEngine-imgui
    PUBLIC
        SDL3::SDL3
        Vulkan::Vulkan
)
create_module(StrideEngine-imgui SHARED)

find_package(Vulkan REQUIRED)
add_dependencies(StrideEngine-imgui
    PRIVATE
        SDL3::SDL3
        Vulkan::Vulkan
)
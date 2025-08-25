find_package(Vulkan REQUIRED)
create_module(StrideEngine-imgui PUBLIC SHARED
        SDL3::SDL3
        Vulkan::Vulkan
)
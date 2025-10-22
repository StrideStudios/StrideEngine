create_module(StrideEngine-imgui SHARED)
target_include_directories(StrideEngine-imgui PUBLIC "include/imgui")

find_package(Vulkan REQUIRED)
add_dependencies(StrideEngine-imgui
    PRIVATE
        SDL3::SDL3
        Vulkan::Vulkan
)
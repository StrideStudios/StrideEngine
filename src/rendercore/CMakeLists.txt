create_module(StrideEngine-rendercore SHARED)

find_package(Vulkan REQUIRED COMPONENTS dxc)
add_dependencies(StrideEngine-rendercore
    PRIVATE
        SDL3::SDL3
        fastgltf::fastgltf
        freetype
        basisu_encoder
        meshoptimizer
        Vulkan::dxc_lib
    PUBLIC
        vk-bootstrap::vk-bootstrap
        Vulkan::Vulkan

        StrideEngine-basic
)

# Basis universal doesn't include itself by default
target_include_directories(StrideEngine-rendercore PUBLIC
        "../../third_party/basis_universal"
)
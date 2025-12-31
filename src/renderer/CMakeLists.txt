create_module(StrideEngine-renderer SHARED)

add_dependencies(StrideEngine-renderer
    PRIVATE
        vk-bootstrap::vk-bootstrap
        SDL3::SDL3

        StrideEngine-imgui
    PUBLIC
        StrideEngine-basic
        StrideEngine-rendercore
        StrideEngine-engine
)

# Basis universal doesn't include itself by default
target_include_directories(StrideEngine-renderer PUBLIC
        "../../third_party/basis_universal"
)
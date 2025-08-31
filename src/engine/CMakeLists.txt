create_module(StrideEngine-engine SHARED)

add_public_dependencies(StrideEngine-engine
        SDL3::SDL3

        StrideEngine-basic
        StrideEngine-imgui
        StrideEngine-rendercore
)
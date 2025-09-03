create_module(StrideEngine-engine SHARED)

add_dependencies(StrideEngine-engine
    PUBLIC
        SDL3::SDL3

        StrideEngine-basic
        StrideEngine-imgui
        StrideEngine-rendercore
)
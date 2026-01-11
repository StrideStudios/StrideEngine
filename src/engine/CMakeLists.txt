create_module(StrideEngine-engine SHARED)

add_dependencies(StrideEngine-engine
    PRIVATE
        SDL3::SDL3

        StrideEngine-rendercore
        StrideEngine-imgui
    PUBLIC
        StrideEngine-basic
        StrideEngine-VRI
)
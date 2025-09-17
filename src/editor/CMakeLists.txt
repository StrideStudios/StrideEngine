create_module(StrideEngine-editor SHARED)

add_dependencies(StrideEngine-editor
    PUBLIC
        StrideEngine-basic
        StrideEngine-rendercore
        StrideEngine-renderer
)
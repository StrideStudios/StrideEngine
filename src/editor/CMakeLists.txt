create_module(StrideEngine-editor SHARED)

add_dependencies(StrideEngine-editor
    PUBLIC
        StrideEngine-basic
        StrideEngine-rendercore
        StrideEngine-engine
        StrideEngine-scene
        StrideEngine-imgui
        StrideEngine-renderer
)
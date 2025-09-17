create_module(StrideEngine-scene SHARED)

add_dependencies(StrideEngine-scene
    PUBLIC
        StrideEngine-basic
        StrideEngine-rendercore
        StrideEngine-engine
)
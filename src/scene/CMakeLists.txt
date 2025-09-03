create_module(StrideEngine-scene SHARED)

add_dependencies(StrideEngine-scene
    PUBLIC
        freetype

        StrideEngine-basic
        StrideEngine-engine
)
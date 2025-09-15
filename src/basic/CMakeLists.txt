create_precompiled_module(StrideEngine-basic SHARED)

add_dependencies(StrideEngine-basic
    PUBLIC
        glm::glm
        fmt::fmt
        Tracy::TracyClient
)
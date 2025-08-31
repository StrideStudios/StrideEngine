create_module(StrideEngine-basic SHARED)

add_public_dependencies(StrideEngine-basic
        glm::glm
        fmt::fmt
        Tracy::TracyClient
)

# Since basic doesnt rely on other modules, it can be added to a pch entirely
target_precompile_headers(StrideEngine-basic PUBLIC
        include/core/Common.h
        include/control/ResourceManager.h
        include/control/ClassManager.h
        include/core/Paths.h
        include/core/Threading.h
        include/core/Archive.h
        include/core/Hashing.h
)
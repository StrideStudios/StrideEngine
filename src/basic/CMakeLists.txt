create_module(StrideEngine-basic SHARED)

add_dependencies(StrideEngine-basic
    PUBLIC
        glm::glm
        fmt::fmt
        Tracy::TracyClient
)

# Since basic doesnt rely on other modules, it can be added to a pch entirely
target_precompile_headers(StrideEngine-basic PUBLIC
        include/core/Object.h
        include/core/Common.h
        include/core/Singleton.h
        include/control/ResourceManager.h
        include/core/Factory.h
        include/core/Class.h
        include/core/Registry.h
        include/control/ClassManager.h
        include/core/Paths.h
        include/core/Threading.h
        include/core/Archive.h
        include/core/Hashing.h
)
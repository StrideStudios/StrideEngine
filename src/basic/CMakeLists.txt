add_library(StrideEngine-basic SHARED
        include/core/Hashing.h
        include/core/Common.h
        include/core/Archive.h
        include/core/Threading.h
        include/core/Paths.h

        include/control/ClassManager.h
        src/control/ClassManager.cpp
        include/control/ResourceManager.h
)

set_target_properties(StrideEngine-basic PROPERTIES LINKER_LANGUAGE CXX)

target_include_directories(StrideEngine-basic PUBLIC
        "include"
)

target_link_libraries(StrideEngine-basic PUBLIC
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
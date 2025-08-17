add_library(StrideEngine-core SHARED
        include/Hashing.h
        include/Common.h
        include/Archive.h
        src/Archive.cpp
        include/Threading.h
        include/Paths.h
)

set_target_properties(StrideEngine-core PROPERTIES LINKER_LANGUAGE CXX)

target_include_directories(StrideEngine-core PUBLIC
        "include"
)

target_link_libraries(StrideEngine-core PUBLIC
        glm::glm
        fmt::fmt
        Tracy::TracyClient
)

# Since core doesnt rely on other modules, it can be added to a pch entirely
target_precompile_headers(StrideEngine-core PUBLIC
        include/Common.h
        include/Paths.h
        include/Threading.h
        include/Archive.h
        include/Hashing.h
)
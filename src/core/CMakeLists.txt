add_library(core SHARED
        # Core

        include/SectionManager.h
        include/ResourceManager.h
        include/Hashing.h
        include/Common.h
        include/Archive.h
        include/Threading.h
        include/Paths.h
        src/SectionManager.cpp
)

set_target_properties(core PROPERTIES LINKER_LANGUAGE CXX)

target_link_libraries(core
    PUBLIC
        glm::glm
        fmt::fmt
        Tracy::TracyClient
)
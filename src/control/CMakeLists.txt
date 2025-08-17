add_library(StrideEngine-control SHARED
        include/ResourceManager.h
        include/ModuleManager.h
        src/ModuleManager.cpp
        include/Modules.h
)

set_target_properties(StrideEngine-control PROPERTIES LINKER_LANGUAGE CXX)

target_include_directories(StrideEngine-control PUBLIC
        "include"
        "../core/include"
)

target_link_libraries(StrideEngine-control PUBLIC
        StrideEngine-core
)

target_precompile_headers(StrideEngine-control PUBLIC
        include/ResourceManager.h
        include/ModuleManager.h
        include/Modules.h
)
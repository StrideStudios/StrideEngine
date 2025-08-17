add_library(StrideEngine-engine SHARED
        include/Engine.h
        src/Engine.cpp
        include/EngineSettings.h
        src/EngineSettings.cpp
        include/Viewport.h
        include/Input.h
        src/Input.cpp
        src/Viewport.cpp
)

set_target_properties(StrideEngine-engine PROPERTIES LINKER_LANGUAGE CXX)

target_include_directories(StrideEngine-engine PUBLIC
        "include"
        "../basic/include"
        "../imgui/include"
)

target_link_libraries(StrideEngine-engine PUBLIC
        SDL3::SDL3
        vk-bootstrap::vk-bootstrap

        StrideEngine-basic
        StrideEngine-imgui
)
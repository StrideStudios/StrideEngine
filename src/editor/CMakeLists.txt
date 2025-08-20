add_library(StrideEngine-editor SHARED
        include/EditorRenderer.h
        src/EditorRenderer.cpp
)

set_target_properties(StrideEngine-editor PROPERTIES LINKER_LANGUAGE CXX)

target_include_directories(StrideEngine-editor PUBLIC
        "include"
        "../basic/include"
        "../rendercore/include"
        "../engine/include"
        "../scene/include"
        "../imgui/include"
        "../renderer/include"
)

target_link_libraries(StrideEngine-editor PUBLIC
        StrideEngine-basic
        StrideEngine-rendercore
        StrideEngine-engine
        StrideEngine-scene
        StrideEngine-imgui
        StrideEngine-renderer
)
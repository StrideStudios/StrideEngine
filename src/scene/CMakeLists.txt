add_library(StrideEngine-scene SHARED
        include/Scene.h
        include/Camera.h
        src/Camera.cpp
        include/SceneObject.h
        src/SceneObject.cpp
        include/Sprite.h
        include/Widget.h
        src/Widget.cpp
        src/Scene.cpp
)

set_target_properties(StrideEngine-scene PROPERTIES LINKER_LANGUAGE CXX)

target_include_directories(StrideEngine-scene PUBLIC
        "include"
        "../core/include"
        "../control/include"
        "../engine/include"
)

target_link_libraries(StrideEngine-scene PUBLIC
        StrideEngine-core
        StrideEngine-control
        StrideEngine-engine
)
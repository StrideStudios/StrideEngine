add_library(engine SHARED
        # Engine

        include/Engine.h
        src/Engine.cpp
        include/Scene.h
        include/EngineSettings.h
        src/EngineSettings.cpp
        include/Viewport.h
        include/Camera.h
        src/Camera.cpp
        include/SceneObject.h
        src/SceneObject.cpp
        include/Sprite.h
        include/Input.h
        src/Input.cpp
        src/Viewport.cpp
        include/Widget.h
        src/Widget.cpp
)

set_target_properties(engine PROPERTIES LINKER_LANGUAGE CXX)

target_link_libraries(engine
    PUBLIC
        SDL3::SDL3
        vk-bootstrap::vk-bootstrap

        core
        imgui
        #renderer
)
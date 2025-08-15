add_library(imgui SHARED
        # Dear ImGui

        include/imconfig.h
        include/imgui.h
        include/imgui_internal.h
        include/imstb_rectpack.h
        include/imstb_textedit.h
        include/imstb_truetype.h
        include/imgui_impl_vulkan.h
        include/imgui_impl_sdl3.h
        src/imgui.cpp
        src/imgui_demo.cpp
        src/imgui_draw.cpp
        src/imgui_tables.cpp
        src/imgui_widgets.cpp
        src/imgui_impl_vulkan.cpp
        src/imgui_impl_sdl3.cpp
        include/imgui_stdlib.h
        src/imgui_stdlib.cpp
)

set_target_properties(imgui PROPERTIES LINKER_LANGUAGE CXX)

find_package(Vulkan REQUIRED)
target_link_libraries(imgui
    PUBLIC
        SDL3::SDL3
        Vulkan::Vulkan
)
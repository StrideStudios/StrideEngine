add_subdirectory(basic)
add_subdirectory(imgui)
add_subdirectory(scene)
add_subdirectory(rendercore)
add_subdirectory(engine)
add_subdirectory(renderer)
add_subdirectory(editor)

add_executable(StrideEngine Application.cpp)

target_link_libraries(StrideEngine PUBLIC
        StrideEngine-basic
        StrideEngine-imgui
        StrideEngine-scene
        StrideEngine-rendercore
        StrideEngine-engine
        StrideEngine-renderer
        StrideEngine-editor
)
target_compile_definitions(TracyClient PRIVATE TRACY_ENABLE>)
target_compile_definitions(StrideEngine PRIVATE TRACY_ENABLE>)
# Tell cmake to copy dlls to the target directory
add_custom_command(TARGET StrideEngine POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_RUNTIME_DLLS:StrideEngine> $<TARGET_FILE_DIR:StrideEngine>
        COMMAND_EXPAND_LISTS
)
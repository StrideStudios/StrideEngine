add_subdirectory(core)
add_subdirectory(control)
add_subdirectory(imgui)
add_subdirectory(scene)
add_subdirectory(engine)
add_subdirectory(renderer)

add_executable(StrideEngine Application.cpp)

target_link_libraries(StrideEngine PUBLIC
        StrideEngine-core
        StrideEngine-control
        StrideEngine-imgui
        StrideEngine-scene
        StrideEngine-engine
        StrideEngine-renderer
)
target_compile_definitions(TracyClient PRIVATE TRACY_ENABLE>)
target_compile_definitions(StrideEngine PRIVATE TRACY_ENABLE>)
# Tell cmake to copy dlls to the target directory
add_custom_command(TARGET StrideEngine POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_RUNTIME_DLLS:StrideEngine> $<TARGET_FILE_DIR:StrideEngine>
        COMMAND_EXPAND_LISTS
)
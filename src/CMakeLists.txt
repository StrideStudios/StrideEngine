# Function for creating modules in subdirectories
function(create_module TARGET_NAME TYPE LIBRARY_TYPE)

    # Get headers and sources
    # Configure depends allows it to check if new files have been added
    # It is slow on large projects but works for now, a custom hashing solution could work better down the line
    file(GLOB_RECURSE HEADERS CONFIGURE_DEPENDS "include" "*.h")
    file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS "src" "*.cpp")

    # Create library with headers and sources
    add_library(${TARGET_NAME} ${LIBRARY_TYPE}
            ${HEADERS}
            ${SOURCES}
    )

    # Ensure target is using CXX
    set_target_properties(${TARGET_NAME} PROPERTIES LINKER_LANGUAGE CXX)

    # If it finds an include directory, include it
    if(EXISTS "include")
        target_include_directories(${TARGET_NAME} ${TYPE} "include")
    endif()

    # Each module the target wishes to link with is an extra argument
    foreach(arg IN LISTS ARGN)

        # Link with other target
        target_link_libraries(${TARGET_NAME} ${TYPE} ${arg})

        # Get other target's source directory and create an include directory for it
        get_target_property(TARGET_DIR ${arg} CMAKE_CURRENT_SOURCE_DIR)
        set(TARGET_INCLUDE_DIR "${TARGET_DIR}/include")

        # if it finds an include directory, include it
        if(EXISTS ${TARGET_INCLUDE_DIR})
            target_include_directories(${TARGET_NAME} ${TYPE} ${TARGET_INCLUDE_DIR})
        endif()
    endforeach()
endfunction()

# Since subdirectories need to be in order of dependency, as well as linked libraries, its best to do them together
function(subdirectory_link TARGET_NAME TYPE)
    foreach(arg IN LISTS ARGN)
        add_subdirectory(${arg})
        target_link_libraries(${TARGET_NAME} ${TYPE} "StrideEngine-${arg}")
    endforeach()
endfunction()

add_executable(StrideEngine Application.cpp)

subdirectory_link(StrideEngine PUBLIC
        basic
        imgui
        rendercore
        engine
        scene
        renderer
        editor
)

# Tell cmake to copy dlls to the target directory
add_custom_command(TARGET StrideEngine POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_RUNTIME_DLLS:StrideEngine> $<TARGET_FILE_DIR:StrideEngine>
        COMMAND_EXPAND_LISTS
)
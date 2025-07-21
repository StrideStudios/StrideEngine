find_program(GLSLC glslc
        HINTS /usr/bin /usr/local/bin $ENV{VULKAN_SDK}/bin/ $ENV{VULKAN_SDK}/bin32/
)

find_program(GLSLVALIDATOR glslangValidator
        HINTS /usr/bin /usr/local/bin $ENV{VULKAN_SDK}/bin/ $ENV{VULKAN_SDK}/bin32/
)

# TODO: This works fine for now, but runtime compiling with glslang would be much better
# Slang would be nice but it is still quite buggy and new
function (compile_shaders target)
    set(SHADER_FILES ${ARGN}) #Other arguments are the shader files
    set(SHADERS_DIR "${PROJECT_SOURCE_DIR}/shaders")
    set(SHADERS_TARGET_DIR "${PROJECT_BINARY_DIR}/shaders")
    file(MAKE_DIRECTORY "${SHADERS_DIR}")
    foreach (SHADER_FILENAME IN LISTS SHADER_FILES)
        set(SHADER_PATH "${SHADERS_DIR}/${SHADER_FILENAME}")
        set(SHADER_SPIRV_PATH "${SHADERS_TARGET_DIR}/${SHADER_FILENAME}.spv")
        set(DEPFILE "${SHADER_SPIRV_PATH}.d")
        message("-- Building Shader: ${SHADER_FILENAME} at ${SHADER_PATH}")
        add_custom_command(
                OUTPUT "${SHADER_SPIRV_PATH}"
                COMMAND ${GLSLC} "${SHADER_PATH}" -o "${SHADER_SPIRV_PATH}" -MD -MF ${DEPFILE} -g
                DEPENDS "${SHADER_PATH}"
                DEPFILE "${DEPFILE}"
        )
        list(APPEND SPIRV_BINARY_FILES ${SHADER_SPIRV_PATH})
    endforeach()

    message("-- Shader Build Complete, files written to ${SHADERS_TARGET_DIR}")
    message("--")
    set(shaders_target_name "${target}_build_shaders")
    add_custom_target(${shaders_target_name}
            DEPENDS ${SPIRV_BINARY_FILES}
    )

    add_dependencies(${target} ${shaders_target_name})
endfunction()
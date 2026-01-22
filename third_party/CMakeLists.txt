option(SIMPLESTL_INIT "" ON)
option(SIMPLESTL_DESTROY "" ON)
option(SIMPLESTL_RELEASE "" ON)
add_subdirectory(simplestl)

add_subdirectory(glm)
add_subdirectory(fmt)
add_subdirectory(sdl)

add_subdirectory(vk-bootstrap)

add_subdirectory(fastgltf)
add_subdirectory(meshoptimizer)
add_subdirectory(freetype)

option(BASISU_SUPPORT_SSE "" ON)
option(BASISU_SUPPORT_OPENCL "" ON)

# Disable compression formats we wont use
option(BASISD_SUPPORT_ASTC "" OFF)
option(BASISD_SUPPORT_ATC "" OFF)

add_subdirectory(basis_universal)

option(TRACY_ENABLE "" ON)
option(TRACY_ON_DEMAND "" ON)
add_subdirectory(tracy)

target_compile_definitions(TracyClient PUBLIC TRACY_EXPORTS)
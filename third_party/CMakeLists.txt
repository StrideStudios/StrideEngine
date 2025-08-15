add_subdirectory(glm)
add_subdirectory(fmt)
add_subdirectory(sdl)
add_subdirectory(vk-bootstrap)
add_subdirectory(glslang)
add_subdirectory(fastgltf)
add_subdirectory(meshoptimizer)

option(BASISU_SUPPORT_SSE "" ON)
option(BASISU_SUPPORT_OPENCL "" ON)

# Disable compression formats we wont use
option(BASISD_SUPPORT_ASTC "" OFF)
option(BASISD_SUPPORT_ATC "" OFF)

add_subdirectory(basis_universal)

option(TRACY_ENABLE "" ON)
option(TRACY_ON_DEMAND "" ON)
add_subdirectory(tracy)
# CMake Parent File Requirements
cmake_minimum_required(VERSION 3.31)

set(CMAKE_CXX_STANDARD 23)
# Temporary solution until i get a dll copying thing for some of the stinkier dlls
# set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static-libstdc++ -static-libgcc --static")

set(BUILD_DIR PROJECT_BINARY_DIR)

# Project Definition

project(
    StrideEngine
    VERSION 1.0.0
    LANGUAGES CXX C
)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# Code

add_subdirectory(third_party)
add_subdirectory(src)
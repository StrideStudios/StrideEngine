# CMake Parent File Requirements
cmake_minimum_required(VERSION 3.31)

set(CMAKE_CXX_STANDARD 23)
option(BUILD_SHARED_LIBS "Build using shared libraries" ON)

# Project Definition

project(
    StrideEngine
    VERSION 1.0.0
    LANGUAGES CXX C
)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")
set(BUILD_DIR PROJECT_BINARY_DIR)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# Code

add_subdirectory(third_party)
add_subdirectory(src)
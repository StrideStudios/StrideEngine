# CMake Parent File Requirements
cmake_minimum_required(VERSION 3.31)

set(CMAKE_CXX_STANDARD 23)

# Project Definition

project(
    StrideEngine
    VERSION 1.0.0
    LANGUAGES CXX C
)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

if (NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/third_party/SDL/CMakeLists.txt")
    include(FetchSubmodules)
    fetch_submodules()
endif()

# Code

add_subdirectory(third_party)
add_subdirectory(src)
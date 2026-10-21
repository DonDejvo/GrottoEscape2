cmake_minimum_required(VERSION 3.22)
project(grotto_escape)
set(CMAKE_CXX_STANDARD 20)

# Options (must be defined before Dependencies is included)
option(BUILD_GAME "Build the game executable" ON)
if(EMSCRIPTEN)
    option(USE_GLES "Build web target with OpenGL ES instead of WebGPU" OFF)
else()
    set(USE_GLES OFF)   # native is always WebGPU
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
include(Dependencies)

add_subdirectory(src/assets)
if(NOT EMSCRIPTEN)
    add_subdirectory(tools/asset_packer)
endif()
if(BUILD_GAME)
    add_subdirectory(src)
endif()
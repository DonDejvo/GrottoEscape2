cmake_minimum_required(VERSION 3.22) # CMake version check
project(grotto_escape)                         # Create project "app"
set(CMAKE_CXX_STANDARD 20)           # Enable C++20 standard

include(FetchContent)

if(EMSCRIPTEN)
    option(USE_GLES "Build web target with OpenGL ES instead of WebGPU" OFF)
else()
    set(USE_GLES OFF)  # native is always WebGPU
endif()

set(COMMON_SOURCES
    "grotto_escape.cpp" 
    "window.h" 
    "window.cpp"
    "renderer.h"
    "renderer.cpp"
    "asset_manager.h"
    "asset_manager.cpp" 
    "entity.h" 
    "entity.cpp"
    "sprite.h" 
    "sprite.cpp" 
    "tilemap.h" 
    "tilemap.cpp"
    "game_state.h"
    "game_state.cpp" 
    "asset_archive.h" 
    "input.h" 
    "input.cpp"
    "engine.h" 
    "engine.cpp" 
    "emc_export.cpp" 
    "backend.h" 
    "backend.cpp"
    "ui.h"
    "ui.cpp"
    "constants.h" 
    "text_analyzer.h" 
    "text_analyzer.cpp")

if(USE_GLES)
    list(APPEND COMMON_SOURCES "gles_backend.h" "gles_backend.cpp")
else()
    list(APPEND COMMON_SOURCES "webgpu_backend.h" "webgpu_backend.cpp")
endif()

add_executable(grotto_escape ${COMMON_SOURCES})

if(USE_GLES)
    target_compile_definitions(grotto_escape PRIVATE BACKEND_GLES)
else()
    target_compile_definitions(grotto_escape PRIVATE BACKEND_WEBGPU)
endif()

if (NOT EMSCRIPTEN)
    add_executable(asset_packer
        "asset_packer.cpp"
        "asset_archive.h")
endif()

if(NOT USE_GLES)
    set(DAWN_FETCH_DEPENDENCIES ON)
    add_subdirectory("dawn" EXCLUDE_FROM_ALL)
endif()

FetchContent_Declare(
	glm
	GIT_REPOSITORY https://github.com/g-truc/glm.git
	GIT_TAG 8d1fd52e5ab5590e2c81768ace50c72bae28f2ed #refs/tags/1.0.3
)
FetchContent_MakeAvailable(glm)

FetchContent_Declare(
    json 
    URL https://github.com/nlohmann/json/releases/download/v3.12.0/json.tar.xz
)
FetchContent_MakeAvailable(json)

target_include_directories(grotto_escape PRIVATE stb)

set(LIBS glm::glm nlohmann_json::nlohmann_json)

set(EMC_LINK_OPTS
    "-sUSE_GLFW=3"
    "-sEXPORTED_RUNTIME_METHODS=['ccall','cwrap']"
    "--shell-file=${CMAKE_SOURCE_DIR}/html/shell_v0.5.6.html"
    "--preload-file=${CMAKE_SOURCE_DIR}/data/game.dat@data/game.dat"
)

if(EMSCRIPTEN)
    set_target_properties(grotto_escape PROPERTIES SUFFIX ".html")

    if(USE_GLES)
        target_link_libraries(grotto_escape PRIVATE ${LIBS})
        target_link_options(grotto_escape PRIVATE 
            ${EMC_LINK_OPTS}
            "-sUSE_WEBGL2=1"
            "-sFULL_ES3=1"
        )
    else()
        target_link_libraries(grotto_escape PRIVATE emdawnwebgpu_cpp webgpu_glfw ${LIBS})
        target_link_options(grotto_escape PRIVATE 
            ${EMC_LINK_OPTS}
            "-sASYNCIFY=1"
        )
    endif()

else()
    target_link_libraries(grotto_escape PRIVATE webgpu_dawn webgpu_glfw glfw ${LIBS})
endif()

if(WIN32)
    set(D3DCOMPILER_DLL "C:/Program Files (x86)/Windows Kits/10/Redist/D3D/x64/d3dcompiler_47.dll")
    add_custom_command(TARGET grotto_escape POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${D3DCOMPILER_DLL}"
        "$<TARGET_FILE_DIR:grotto_escape>"
    )
endif()
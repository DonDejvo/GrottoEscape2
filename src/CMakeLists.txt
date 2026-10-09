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

target_sources(grotto_escape PRIVATE ${COMMON_SOURCES})
if (NOT EMSCRIPTEN)
    target_sources(asset_packer PRIVATE "asset_packer.cpp" "asset_archive.h")
endif()
add_subdirectory(backend)

file(GLOB GAME_SOURCES CONFIGURE_DEPENDS *.cpp)
add_executable(grotto_escape ${GAME_SOURCES})
target_link_libraries(grotto_escape PRIVATE backend assets stb)

if(EMSCRIPTEN)
    if(NOT EXISTS "${CMAKE_SOURCE_DIR}/data/game.dat")
        message(FATAL_ERROR "data/game.dat not found. Run asset_packer first.")
    endif()

    set_target_properties(grotto_escape PROPERTIES SUFFIX ".html" OUTPUT_NAME "index")
    target_link_options(grotto_escape PRIVATE
        "-sEXPORTED_RUNTIME_METHODS=['ccall','cwrap']"
        "--shell-file=${CMAKE_SOURCE_DIR}/html/shell_v0.5.6.html"
        "--preload-file=${CMAKE_SOURCE_DIR}/data/game.dat@data/game.dat"
    )
endif()

if(WIN32)
    set(D3DCOMPILER_DLL "C:/Program Files (x86)/Windows Kits/10/Redist/D3D/x64/d3dcompiler_47.dll")
    add_custom_command(TARGET grotto_escape POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${D3DCOMPILER_DLL}"
        "$<TARGET_FILE_DIR:grotto_escape>"
    )
endif()
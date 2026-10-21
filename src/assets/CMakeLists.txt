add_library(assets STATIC 
    asset_archive.h
    asset_manager.cpp
    asset_manager.h)

target_include_directories(assets PUBLIC ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(assets PUBLIC glm::glm nlohmann_json::nlohmann_json)
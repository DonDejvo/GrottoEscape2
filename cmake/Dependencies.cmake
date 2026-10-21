include(FetchContent)

FetchContent_Declare(
	glm
	GIT_REPOSITORY https://github.com/g-truc/glm.git
	GIT_TAG 8d1fd52e5ab5590e2c81768ace50c72bae28f2ed #refs/tags/1.0.3
)

FetchContent_Declare(
    json 
    URL https://github.com/nlohmann/json/releases/download/v3.12.0/json.tar.xz
)
FetchContent_MakeAvailable(glm json)

add_library(stb INTERFACE)
target_include_directories(stb INTERFACE ${CMAKE_SOURCE_DIR}/stb)

if(BUILD_GAME AND NOT USE_GLES)
    set(DAWN_FETCH_DEPENDENCIES ON)
    add_subdirectory(${CMAKE_SOURCE_DIR}/dawn ${CMAKE_BINARY_DIR}/dawn EXCLUDE_FROM_ALL)
endif()
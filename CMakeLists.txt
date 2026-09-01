cmake_minimum_required(VERSION 3.20)
project(VRAMDungeon VERSION 1.0.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

include(FetchContent)

# -----------------------------------------------------------------------------
# Common Dependencies (GLM, tinyobjloader, stb)
# -----------------------------------------------------------------------------

# 1. GLM
FetchContent_Declare(
    glm
    URL ${CMAKE_SOURCE_DIR}/libs/glm.zip
)
FetchContent_GetProperties(glm)
if(NOT glm_POPULATED)
    FetchContent_Populate(glm)
    add_library(glm INTERFACE)
    target_include_directories(glm INTERFACE ${glm_SOURCE_DIR})
endif()

# 2. tinyobjloader
FetchContent_Declare(
    tinyobjloader
    URL ${CMAKE_SOURCE_DIR}/libs/tinyobjloader.zip
)
FetchContent_MakeAvailable(tinyobjloader)

# 3. stb
FetchContent_Declare(
    stb
    URL ${CMAKE_SOURCE_DIR}/libs/stb.zip
)
FetchContent_GetProperties(stb)
if(NOT stb_POPULATED)
    FetchContent_Populate(stb)
    add_library(stb INTERFACE)
    target_include_directories(stb INTERFACE ${stb_SOURCE_DIR})
endif()

# -----------------------------------------------------------------------------
# Platform Specific Dependencies (SFML & GLAD for Desktop)
# -----------------------------------------------------------------------------

if(NOT EMSCRIPTEN)
    # SFML (Window, System)
    set(SFML_BUILD_WINDOW ON CACHE BOOL "" FORCE)
    set(SFML_BUILD_GRAPHICS OFF CACHE BOOL "" FORCE)
    set(SFML_BUILD_NETWORK OFF CACHE BOOL "" FORCE)
    set(SFML_BUILD_AUDIO OFF CACHE BOOL "" FORCE)
    set(SFML_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(SFML_BUILD_DOC OFF CACHE BOOL "" FORCE)

    FetchContent_Declare(
        sfml
        URL ${CMAKE_SOURCE_DIR}/libs/sfml.zip
    )
    FetchContent_MakeAvailable(sfml)

    # GLAD
    FetchContent_Declare(
        glad
        URL ${CMAKE_SOURCE_DIR}/libs/glad.zip
    )
    FetchContent_MakeAvailable(glad)
    set(GLAD_SOURCE "${glad_SOURCE_DIR}/src/glad.c")
else()
    set(GLAD_SOURCE "")
endif()

# -----------------------------------------------------------------------------
# Main Executable
# -----------------------------------------------------------------------------

file(GLOB_RECURSE SOURCES "src/*.cpp")
file(GLOB_RECURSE HEADERS "src/*.h" "src/*.hpp")

add_executable(${PROJECT_NAME} 
    ${SOURCES} 
    ${HEADERS} 
    ${GLAD_SOURCE}
)

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE 
    src
    src/core
    src/core/glad_shim
    src/entities
    src/combat
    src/world
    src/graphics
    src/ui
    src/inventory
)

if(NOT EMSCRIPTEN)
    target_include_directories(${PROJECT_NAME} PRIVATE 
        ${glad_SOURCE_DIR}/include
    )
    target_link_libraries(${PROJECT_NAME} PRIVATE
        sfml-window
        sfml-system
        glm
        glad
        tinyobjloader
        stb
    )

    # Copy assets and config.json to bin folder (Post-build)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/assets
        ${CMAKE_BINARY_DIR}/bin/assets
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/config.json
        ${CMAKE_BINARY_DIR}/bin/config.json
    )
else()
    target_link_libraries(${PROJECT_NAME} PRIVATE
        glm
        tinyobjloader
        stb
    )
    set_target_properties(${PROJECT_NAME} PROPERTIES
        OUTPUT_NAME "index"
        SUFFIX ".html"
    )
    set_property(TARGET ${PROJECT_NAME} PROPERTY LINK_FLAGS
        "-sUSE_GLFW=3 -sFULL_ES3=1 -sMAX_WEBGL_VERSION=2 -sMIN_WEBGL_VERSION=2 -sALLOW_MEMORY_GROWTH=1 -sINITIAL_MEMORY=134217728 --preload-file \"${CMAKE_SOURCE_DIR}/assets@assets\" --preload-file \"${CMAKE_SOURCE_DIR}/config.json@config.json\" --shell-file \"${CMAKE_SOURCE_DIR}/web/shell.html\""
    )
endif()

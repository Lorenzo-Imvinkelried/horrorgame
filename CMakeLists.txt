cmake_minimum_required(VERSION 3.20)
project(GamePS1Horror VERSION 0.1.0 LANGUAGES CXX C)
# Force CMake Re-scan for Monster & ScentManager

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

include(FetchContent)

# -----------------------------------------------------------------------------
# Dependencies
# -----------------------------------------------------------------------------

# 1. SFML (Window, System, Audio)
# We disable graphics/network to keep it minimal as requested, but Window needs System.
set(SFML_BUILD_WINDOW ON CACHE BOOL "" FORCE)
set(SFML_BUILD_GRAPHICS OFF CACHE BOOL "" FORCE) # Reverting to OpenGL UI due to linker issues
set(SFML_BUILD_NETWORK OFF CACHE BOOL "" FORCE)
set(SFML_BUILD_AUDIO OFF CACHE BOOL "" FORCE)
set(SFML_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(SFML_BUILD_DOC OFF CACHE BOOL "" FORCE)
# set(SFML_BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE) # Revert to default

FetchContent_Declare(
    sfml
    URL ${CMAKE_SOURCE_DIR}/libs/sfml.zip
)
FetchContent_MakeAvailable(sfml)

# 2. GLM
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

# 3. GLAD
FetchContent_Declare(
    glad
    URL ${CMAKE_SOURCE_DIR}/libs/glad.zip
)
FetchContent_MakeAvailable(glad)

# 4. tinyobjloader
FetchContent_Declare(
    tinyobjloader
    URL ${CMAKE_SOURCE_DIR}/libs/tinyobjloader.zip
)
FetchContent_MakeAvailable(tinyobjloader)

# 5. stb
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
# Main Executable
# -----------------------------------------------------------------------------

# Gather source files
file(GLOB_RECURSE SOURCES "src/*.cpp")
# Force re-scan for new files
file(GLOB_RECURSE HEADERS "src/*.h" "src/*.hpp")

# GLAD Source (from fetched content)
set(GLAD_SOURCE "${glad_SOURCE_DIR}/src/glad.c")

add_executable(${PROJECT_NAME} 
    ${SOURCES} 
    ${HEADERS} 
    ${GLAD_SOURCE}
)

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE 
    src
    src/core
    src/entities
    src/combat
    src/world
    src/graphics
    src/ui
    src/inventory
    ${glad_SOURCE_DIR}/include
)

# Link Libraries
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

cmake_minimum_required(VERSION 3.14)
project(MySFMLApp CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include the FetchContent module
include(FetchContent)

# Declare SFML repository and target version
FetchContent_Declare(
    SFML
    GIT_REPOSITORY https://github.com/SFML/SFML.git
    GIT_TAG        3.0.0
)

set(SFML_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(SFML_BUILD_DOCS OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(SFML)

# -------------------------------------------------------------
# 1. Automatically find all .cpp files in src/ (and subfolders)
# -------------------------------------------------------------
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS "src/*.cpp")

# 2. Build the executable using all collected sources
add_executable(MySFMLApp ${SOURCES})

# 3. Link SFML
target_link_libraries(MySFMLApp PRIVATE SFML::Graphics)
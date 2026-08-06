cmake_minimum_required(VERSION 3.21)
project(SuperMario LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# --- Fetch SFML 2.6 ---
include(FetchContent)
FetchContent_Declare(
    SFML
    GIT_REPOSITORY https://github.com/SFML/SFML.git
    GIT_TAG        2.6.x
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(SFML)

# --- Collect source files ---
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS "src/*.cpp")

# --- Executable ---
add_executable(${PROJECT_NAME} ${SOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    sfml-graphics
    sfml-window
    sfml-system
    sfml-audio
)

# --- Standalone collision-resolution test executable ---
option(BUILD_TESTS "Build the standalone collision test executable" ON)
if(BUILD_TESTS)
    set(TEST_SOURCES ${SOURCES})
    list(REMOVE_ITEM TEST_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp")
    add_executable(CollisionTests tests/CollisionResolutionTests.cpp ${TEST_SOURCES})
    target_include_directories(CollisionTests PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    )
    target_link_libraries(CollisionTests PRIVATE
        sfml-graphics
        sfml-window
        sfml-system
        sfml-audio
    )
    enable_testing()
    add_test(NAME CollisionResolutionTests COMMAND CollisionTests)
endif()

# --- Copy assets to build directory ---
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/assets
        $<TARGET_FILE_DIR:${PROJECT_NAME}>/assets
)

# --- Windows: copy SFML DLLs & OpenAL ---
if(WIN32)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:sfml-graphics>
            $<TARGET_FILE:sfml-window>
            $<TARGET_FILE:sfml-system>
            $<TARGET_FILE:sfml-audio>
            $<TARGET_FILE_DIR:${PROJECT_NAME}>
    )
    if(EXISTS "${sfml_SOURCE_DIR}/extlibs/bin/x64/openal32.dll")
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${sfml_SOURCE_DIR}/extlibs/bin/x64/openal32.dll"
                $<TARGET_FILE_DIR:${PROJECT_NAME}>
        )
    endif()
endif()
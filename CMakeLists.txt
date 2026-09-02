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
    set_tests_properties(CollisionResolutionTests PROPERTIES
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    )

    add_executable(ProgressionTests tests/ProgressionTests.cpp ${TEST_SOURCES})
    target_include_directories(ProgressionTests PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    )
    target_link_libraries(ProgressionTests PRIVATE
        sfml-graphics
        sfml-window
        sfml-system
        sfml-audio
    )
    add_test(NAME ProgressionTests COMMAND ProgressionTests)
    set_tests_properties(ProgressionTests PROPERTIES
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    )

    add_executable(NavigationTests tests/NavigationTests.cpp ${TEST_SOURCES})
    target_include_directories(NavigationTests PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    )
    target_link_libraries(NavigationTests PRIVATE
        sfml-graphics
        sfml-window
        sfml-system
        sfml-audio
    )
    add_test(NAME NavigationTests COMMAND NavigationTests)
    set_tests_properties(NavigationTests PROPERTIES
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    )

    add_executable(SaveManagerTests tests/SaveManagerTests.cpp ${TEST_SOURCES})
    target_include_directories(SaveManagerTests PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    )
    target_link_libraries(SaveManagerTests PRIVATE
        sfml-graphics
        sfml-window
        sfml-system
        sfml-audio
    )
    add_test(NAME SaveManagerTests COMMAND SaveManagerTests)
    set_tests_properties(SaveManagerTests PROPERTIES
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    )

    add_executable(GameSettingsTests tests/GameSettingsTests.cpp ${TEST_SOURCES})
    target_include_directories(GameSettingsTests PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    )
    target_link_libraries(GameSettingsTests PRIVATE
        sfml-graphics
        sfml-window
        sfml-system
        sfml-audio
    )
    add_test(NAME GameSettingsTests COMMAND GameSettingsTests)
    set_tests_properties(GameSettingsTests PROPERTIES
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    )

    add_executable(MenuAnimationTests tests/MenuAnimationTests.cpp)
    target_include_directories(MenuAnimationTests PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    )
    add_test(NAME MenuAnimationTests COMMAND MenuAnimationTests)
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

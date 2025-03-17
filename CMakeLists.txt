cmake_minimum_required(VERSION 3.30)

# Globals
set(PROJECT_NAME "GPU IPC Project")
set(TARGET_NAME "gpu_ipc")

project(${PROJECT_NAME})

# Options
option(${TARGET_NAME}_BUILD_TESTS "Enable building tests for ${TARGET_NAME}" OFF)
option(${TARGET_NAME}_BUILD_EXAMPLES "Enable building examples for ${TARGET_NAME}" OFF)

# Folder Structure for VS
if (PROJECT_IS_TOP_LEVEL)
    set(FOLDER_TARGET "")
else()
    set(FOLDER_TARGET "${TARGET_NAME}/")
endif()

# ------ DEFAULT OPTIONS FOR SUBPROJECTS ------
# Subprojects added directly to this project should have their options set here

# Environment settings
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# List all project source in these variables
set(SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/main/Consumer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/main/Producer.cpp
)
set(PRIVATE_HEADERS

)
set(PUBLIC_HEADERS

)
# List all GTest files here
# Generally every hpp/cpp combo should have a matching [filename].test.cpp.
# Header-only should also receive test files.
set(TEST_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/Component.test.cpp
)

# -------------- PROJECT LIBRARY --------------
# Create linkable library from project files
# We break this into a library so that the main exe and test project
# share the same artefacts
add_library(${TARGET_NAME}_lib ${SOURCES} ${PRIVATE_HEADERS} ${PUBLIC_HEADERS})
target_include_directories(
    ${TARGET_NAME}_lib
    PUBLIC  ${CMAKE_CURRENT_SOURCE_DIR}/include # All headers in /include should be public
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src     # All headers in /src should be private
)

## External project linkage would go here
add_subdirectory("external/glh")

add_subdirectory("external/cpputils")

target_link_libraries(
    ${TARGET_NAME}_lib
    PUBLIC glh_lib
    PUBLIC cpputils_lib
)

# ------------- PROJECT EXECUTABLE -------------
add_executable(${TARGET_NAME}_Producer src/main/Producer.cpp)
target_link_libraries(${TARGET_NAME}_Producer PRIVATE ${TARGET_NAME}_lib)

add_executable(${TARGET_NAME}_Consumer src/main/Consumer.cpp)
target_link_libraries(${TARGET_NAME}_Consumer PRIVATE ${TARGET_NAME}_lib)

# ------------------ TESTING ------------------
If (${TARGET_NAME}_BUILD_TESTS)
    # Add Google Test
    include(FetchContent)

    # Fetch Google Test
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG main
    )
    FetchContent_MakeAvailable(googletest)

    set_property(TARGET gmock      PROPERTY FOLDER "${FOLDER_TARGET}external/googletest")
    set_property(TARGET gmock_main PROPERTY FOLDER "${FOLDER_TARGET}external/googletest")
    set_property(TARGET gtest      PROPERTY FOLDER "${FOLDER_TARGET}external/googletest")
    set_property(TARGET gtest_main PROPERTY FOLDER "${FOLDER_TARGET}external/googletest")

    enable_testing()

    add_executable(${TARGET_NAME}_tests ${TEST_SOURCES})
    target_link_libraries(
        ${TARGET_NAME}_tests
        PRIVATE GTest::gtest_main
        PRIVATE ${TARGET_NAME}_lib
    )
    target_include_directories(
        ${TARGET_NAME}_tests
        # Explicitly access all headers, regardless of visibility from _lib
        PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include
        PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src
    )
    set_property(TARGET ${TARGET_NAME}_tests PROPERTY FOLDER "${FOLDER_TARGET}")

    include(GoogleTest)
    gtest_discover_tests(${TARGET_NAME}_tests)
endif()

# --------------- MSVC Settings ---------------
if (MSVC)
    # Set Visual Studio startup project as our main target (normally VS will default to BUILD_ALL)
    set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${TARGET_NAME}_Producer)

    # Enable Multiprocessor compile on targets
    target_compile_options(${TARGET_NAME}_lib PRIVATE /MP)
    target_compile_options(${TARGET_NAME}_Producer PRIVATE /MP)
    target_compile_options(${TARGET_NAME}_Consumer PRIVATE /MP)

    if (${TARGET_NAME}_BUILD_TESTS)
        target_compile_options(${TARGET_NAME}_tests PRIVATE /MP)
    endif()

    # Provides folder tree in visual studio filters
    source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/src PREFIX "Source Files" FILES ${SOURCES})
    source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/src PREFIX "Private Header Files" FILES ${PRIVATE_HEADERS})
    source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/include PREFIX "Public Header Files" FILES ${PUBLIC_HEADERS})
    source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/tests PREFIX "Tests" FILES ${TEST_SOURCES})
endif()
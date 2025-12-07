# UltraWeb CMake Build Configuration
# Version: 1.0.0
# Last Modified: 2025-12-04

cmake_minimum_required(VERSION 3.16)
project(UltraWeb VERSION 1.0.0 LANGUAGES CXX)

# C++ Standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compiler flags
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic")
    set(CMAKE_CXX_FLAGS_DEBUG "-g -O0")
    set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")
endif()

# Include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

# ============================================================================
# UltraWeb Core Library
# ============================================================================

set(ULTRAWEB_SOURCES
    core/UltraWebCSSParser.cpp
    core/UltraWebCSSCompiler.cpp
    core/UltraWebUIParser.cpp
    core/UltraWebUICompiler.cpp
    core/UltraWebBundler.cpp
    runtime/UCBLoader.cpp
    runtime/UCSLoader.cpp
    runtime/UltraWebRuntime.cpp
)

set(ULTRAWEB_HEADERS
    include/UltraWebFormats.h
    include/UltraWebCSSParser.h
    include/UltraWebCSSCompiler.h
    include/UltraWebUIParser.h
    include/UltraWebUICompiler.h
    include/UltraWebBundler.h
    runtime/UCBLoader.h
    runtime/UCSLoader.h
    runtime/UltraWebRuntime.h
)

add_library(UltraWeb STATIC ${ULTRAWEB_SOURCES} ${ULTRAWEB_HEADERS})
target_include_directories(UltraWeb PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

# ============================================================================
# Test Executables
# ============================================================================

add_executable(CSSCompilerTests tests/CSSCompilerTests.cpp)
target_link_libraries(CSSCompilerTests PRIVATE UltraWeb)

add_executable(UICompilerTests tests/UICompilerTests.cpp)
target_link_libraries(UICompilerTests PRIVATE UltraWeb)

add_executable(BundlerTests tests/BundlerTests.cpp)
target_link_libraries(BundlerTests PRIVATE UltraWeb)

add_executable(RuntimeTests tests/RuntimeTests.cpp)
target_link_libraries(RuntimeTests PRIVATE UltraWeb)

# Enable testing
enable_testing()
add_test(NAME CSSCompilerTests COMMAND CSSCompilerTests)
add_test(NAME UICompilerTests COMMAND UICompilerTests)
add_test(NAME BundlerTests COMMAND BundlerTests)
add_test(NAME RuntimeTests COMMAND RuntimeTests)

# ============================================================================
# Install Rules
# ============================================================================

install(TARGETS UltraWeb
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

install(FILES ${ULTRAWEB_HEADERS}
    DESTINATION include/UltraWeb
)

# ============================================================================
# Print Configuration Summary
# ============================================================================

message(STATUS "")
message(STATUS "=== UltraWeb Configuration ===")
message(STATUS "Version: ${PROJECT_VERSION}")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "==============================")
message(STATUS "")

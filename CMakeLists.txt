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

option(ULTRAWEB_USE_VIRTUALFS "Use VirtualFS (UltraCanvas module) for .ucpkg LZ4 compression" OFF)
set(ULTRAWEB_VIRTUALFS_DIR "" CACHE PATH "Path to the VirtualFS module (e.g. <UltraCanvas>/VirtualFS)")

option(ULTRAWEB_USE_HERMES "Build the Hermes JS engine backend (requires the Hermes SDK)" OFF)
set(ULTRAWEB_HERMES_SDK_DIR "" CACHE PATH "Hermes SDK install prefix (contains include/ and lib/)")

option(ULTRAWEB_USE_QUICKJS "Build the QuickJS JS engine backend (development/CI engine)" OFF)

set(ULTRAWEB_SOURCES
    core/UltraWebCSSParser.cpp
    core/UltraWebCSSCompiler.cpp
    core/UltraWebUIParser.cpp
    core/UltraWebUICompiler.cpp
    core/UltraWebBundler.cpp
    core/UltraWebCompression.cpp
    runtime/UCBLoader.cpp
    runtime/UCSLoader.cpp
    runtime/UltraWebRuntime.cpp
    runtime/ElementFactory.cpp
    runtime/EventSystem.cpp
    runtime/UCComponents.cpp
    runtime/JSONUtil.cpp
    runtime/StateManager.cpp
    runtime/UCApi.cpp
    runtime/QuickJSEngine.cpp
    runtime/HermesRuntime.cpp
    server/HermesCompiler.cpp
)

set(ULTRAWEB_HEADERS
    include/UltraWebFormats.h
    include/UltraWebCSSParser.h
    include/UltraWebCSSCompiler.h
    include/UltraWebUIParser.h
    include/UltraWebUICompiler.h
    include/UltraWebBundler.h
    include/UltraWebCompression.h
    runtime/UCBLoader.h
    runtime/UCSLoader.h
    runtime/UltraWebRuntime.h
    runtime/ElementFactory.h
    runtime/EventSystem.h
    runtime/UCComponents.h
    runtime/JSEngine.h
    runtime/JSONUtil.h
    runtime/StateManager.h
    runtime/UCApi.h
    runtime/QuickJSEngine.h
    runtime/HermesRuntime.h
    server/HermesCompiler.h
)

add_library(UltraWeb STATIC ${ULTRAWEB_SOURCES} ${ULTRAWEB_HEADERS})
target_include_directories(UltraWeb PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

# ============================================================================
# JavaScript Engine Backends
# ============================================================================
# The engine sources are always in the target; their bodies compile only
# when the matching ULTRAWEB_USE_* define is set.

if(ULTRAWEB_USE_QUICKJS)
    find_path(QUICKJS_INCLUDE_DIR quickjs/quickjs.h)
    find_library(QUICKJS_LIBRARY NAMES quickjs PATHS /usr/lib/quickjs /usr/local/lib/quickjs)
    if(NOT QUICKJS_INCLUDE_DIR OR NOT QUICKJS_LIBRARY)
        message(FATAL_ERROR "ULTRAWEB_USE_QUICKJS: quickjs/quickjs.h or libquickjs not found (install libquickjs / quickjs-devel)")
    endif()
    message(STATUS "QuickJS backend: ${QUICKJS_LIBRARY}")
    target_include_directories(UltraWeb PRIVATE ${QUICKJS_INCLUDE_DIR})
    target_compile_definitions(UltraWeb PUBLIC ULTRAWEB_USE_QUICKJS)
    target_link_libraries(UltraWeb PUBLIC ${QUICKJS_LIBRARY})
    # QuickJS static lib needs libm/pthread/dl on Linux
    if(UNIX AND NOT APPLE)
        target_link_libraries(UltraWeb PUBLIC m pthread ${CMAKE_DL_LIBS})
    endif()
endif()

if(ULTRAWEB_USE_HERMES)
    if(NOT ULTRAWEB_HERMES_SDK_DIR)
        message(FATAL_ERROR "ULTRAWEB_USE_HERMES requires -DULTRAWEB_HERMES_SDK_DIR=<Hermes SDK prefix>")
    endif()
    find_library(HERMES_LIBRARY NAMES hermes libhermes PATHS ${ULTRAWEB_HERMES_SDK_DIR}/lib)
    if(NOT HERMES_LIBRARY)
        message(FATAL_ERROR "ULTRAWEB_USE_HERMES: libhermes not found under ${ULTRAWEB_HERMES_SDK_DIR}/lib")
    endif()
    message(STATUS "Hermes backend: ${HERMES_LIBRARY}")
    target_include_directories(UltraWeb PRIVATE ${ULTRAWEB_HERMES_SDK_DIR}/include)
    target_compile_definitions(UltraWeb PUBLIC ULTRAWEB_USE_HERMES)
    target_link_libraries(UltraWeb PUBLIC ${HERMES_LIBRARY})
endif()

# ============================================================================
# Compression Backend (VirtualFS)
# ============================================================================

if(ULTRAWEB_USE_VIRTUALFS)
    if(NOT ULTRAWEB_VIRTUALFS_DIR)
        message(FATAL_ERROR "ULTRAWEB_USE_VIRTUALFS requires -DULTRAWEB_VIRTUALFS_DIR=<path to UltraCanvas/VirtualFS>")
    endif()
    # .ucpkg payloads use LZ4; make sure VirtualFS builds its LZ4 codec
    set(VIRTUALFS_USE_LZ4 ON CACHE BOOL "LZ4 required for .ucpkg compression" FORCE)
    add_subdirectory(${ULTRAWEB_VIRTUALFS_DIR} ${CMAKE_CURRENT_BINARY_DIR}/VirtualFS)
    target_compile_definitions(UltraWeb PUBLIC ULTRAWEB_USE_VIRTUALFS)
    target_link_libraries(UltraWeb PUBLIC VirtualFS::VirtualFS)
endif()

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

add_executable(ElementFactoryTests tests/ElementFactoryTests.cpp)
target_link_libraries(ElementFactoryTests PRIVATE UltraWeb)

add_executable(EventSystemTests tests/EventSystemTests.cpp)
target_link_libraries(EventSystemTests PRIVATE UltraWeb)

add_executable(JSRuntimeTests tests/JSRuntimeTests.cpp)
target_link_libraries(JSRuntimeTests PRIVATE UltraWeb)

# Enable testing
enable_testing()
add_test(NAME CSSCompilerTests COMMAND CSSCompilerTests)
add_test(NAME UICompilerTests COMMAND UICompilerTests)
add_test(NAME BundlerTests COMMAND BundlerTests)
add_test(NAME RuntimeTests COMMAND RuntimeTests)
add_test(NAME ElementFactoryTests COMMAND ElementFactoryTests)
add_test(NAME EventSystemTests COMMAND EventSystemTests)
add_test(NAME JSRuntimeTests COMMAND JSRuntimeTests)

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

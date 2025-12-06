# Core library CMakeLists.txt
cmake_minimum_required(VERSION 3.16)

# Gather all source files
file(GLOB_RECURSE CORE_SOURCES src/*.cpp)

# Check if we're building for ESP-IDF (has IDF_PATH)
if(DEFINED ENV{IDF_PATH})
    # ESP-IDF build - build lexbor as separate static library
    file(GLOB_RECURSE LEXBOR_SOURCES
        lexbor/source/lexbor/core/*.c
        lexbor/source/lexbor/css/*.c
        lexbor/source/lexbor/css/syntax/*.c
        lexbor/source/lexbor/dom/*.c
        lexbor/source/lexbor/encoding/*.c
        lexbor/source/lexbor/engine/*.c
        lexbor/source/lexbor/html/*.c
        lexbor/source/lexbor/ns/*.c
        lexbor/source/lexbor/punycode/*.c
        lexbor/source/lexbor/selectors/*.c
        lexbor/source/lexbor/style/*.c
        lexbor/source/lexbor/tag/*.c
        lexbor/source/lexbor/unicode/*.c
        lexbor/source/lexbor/url/*.c
        lexbor/source/lexbor/utils/*.c
    )

    # Add POSIX sources but exclude filesystem operations that don't work on ESP32
    file(GLOB LEXBOR_POSIX_SOURCES
        lexbor/source/lexbor/ports/posix/lexbor/core/*.c
    )
    # Remove fs.c which uses dirent.h not available on ESP32
    list(FILTER LEXBOR_POSIX_SOURCES EXCLUDE REGEX ".*fs\\.c$")
    list(APPEND LEXBOR_SOURCES ${LEXBOR_POSIX_SOURCES})

    # Create lexbor static library
    add_library(lexbor_static STATIC ${LEXBOR_SOURCES})
    target_include_directories(lexbor_static PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/lexbor/source)
    target_compile_definitions(lexbor_static PRIVATE LEXBOR_STATIC)
    set_target_properties(lexbor_static PROPERTIES LINKER_LANGUAGE C)
    target_link_libraries(lexbor_static PRIVATE m)

    # Create core library without lexbor sources
    add_library(tactilebrowser_core STATIC ${CORE_SOURCES})
    target_include_directories(tactilebrowser_core PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/lexbor/source
    )
    target_link_libraries(tactilebrowser_core PRIVATE lexbor_static)
else()
    # Regular CMake build - include lexbor sources in core library
    file(GLOB_RECURSE LEXBOR_SOURCES
        lexbor/source/lexbor/core/*.c
        lexbor/source/lexbor/css/*.c
        lexbor/source/lexbor/dom/*.c
        lexbor/source/lexbor/encoding/*.c
        lexbor/source/lexbor/engine/*.c
        lexbor/source/lexbor/html/*.c
        lexbor/source/lexbor/ns/*.c
        lexbor/source/lexbor/punycode/*.c
        lexbor/source/lexbor/selectors/*.c
        lexbor/source/lexbor/style/*.c
        lexbor/source/lexbor/tag/*.c
        lexbor/source/lexbor/unicode/*.c
        lexbor/source/lexbor/url/*.c
        lexbor/source/lexbor/utils/*.c
    )

    # Add POSIX sources but exclude filesystem operations
    file(GLOB LEXBOR_POSIX_SOURCES
        lexbor/source/lexbor/ports/posix/lexbor/core/*.c
    )
    # Remove fs.c which uses dirent.h
    list(FILTER LEXBOR_POSIX_SOURCES EXCLUDE REGEX ".*fs\\.c$")
    list(APPEND LEXBOR_SOURCES ${LEXBOR_POSIX_SOURCES})

    # Create core library with lexbor sources included
    add_library(tactilebrowser_core STATIC ${CORE_SOURCES} ${LEXBOR_SOURCES})
    target_include_directories(tactilebrowser_core PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/lexbor/source
    )
    target_compile_definitions(tactilebrowser_core PRIVATE LEXBOR_STATIC)
endif()

# Set C++ standard
set_target_properties(tactilebrowser_core PROPERTIES CXX_STANDARD 17)
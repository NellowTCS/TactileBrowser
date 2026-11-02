cmake_minimum_required(VERSION 3.20)

# ESP-IDF project inclusion
include($ENV{IDF_PATH}/tools/cmake/project.cmake)

# Tactility SDK path setup
if (DEFINED ENV{TACTILITY_SDK_PATH})
    set(TACTILITY_SDK_PATH $ENV{TACTILITY_SDK_PATH})
else()
    set(TACTILITY_SDK_PATH "../../release/TactilitySDK")
    message(WARNING "⚠️ TACTILITY_SDK_PATH not set, defaulting to ${TACTILITY_SDK_PATH}")
endif()

include("${TACTILITY_SDK_PATH}/TactilitySDK.cmake")
set(EXTRA_COMPONENT_DIRS ${TACTILITY_SDK_PATH})

# Project declaration
project(TactileBrowser LANGUAGES C CXX ASM)
tactility_project(TactileBrowser)

# Global strict compilation flags
add_compile_options(
    -Wall
    -Wextra
    -Wpedantic
    -Werror
    -Wshadow
    -Wconversion
    -Wdouble-promotion
    -Wstrict-prototypes
    -Wno-unused-parameter
)

# Suppress warnings from #include_next in Lexbor
add_compile_options(-Wno-include-next)


# Force standard versions
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
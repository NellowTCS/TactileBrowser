cmake_minimum_required(VERSION 3.20)

project(TactileBrowserDesktop C)

set(CMAKE_C_STANDARD 99)

# Universal build for macOS (Intel + Apple Silicon)
if(APPLE)
  set(CMAKE_OSX_ARCHITECTURES "x86_64;arm64" CACHE STRING "Build universal macOS binary" FORCE)
endif()

# This ensures nice debug output
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Add subdirectory with main app code
add_subdirectory(main)

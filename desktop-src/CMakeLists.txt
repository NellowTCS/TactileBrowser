cmake_minimum_required(VERSION 3.20)

project(TactileBrowserDesktop C)

set(CMAKE_C_STANDARD 99)

# This ensures nice debug output
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Add subdirectory with main app code
add_subdirectory(main)


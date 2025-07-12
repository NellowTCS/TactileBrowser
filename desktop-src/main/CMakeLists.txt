cmake_minimum_required(VERSION 3.15)
project(tactile_browser_test C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Wno-unused-parameter")

# Find SDL2
find_package(SDL2 REQUIRED)

# libcurl
find_package(CURL REQUIRED)

# Add subdirectories for dependencies
add_subdirectory(lvgl)
add_subdirectory(lexbor)

# Include directories
include_directories(
    ${CURL_INCLUDE_DIRS}
    ${SDL2_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}/lvgl
    ${CMAKE_SOURCE_DIR}/lvgl/src
    ${CMAKE_SOURCE_DIR}/lexbor/include
)

# Create executable
add_executable(tactile_browser_test Source/main.c)

# Link libraries
target_link_libraries(tactile_browser_test
    lvgl
    lexbor_static
    ${SDL2_LIBRARIES}
    ${CURL_LIBRARIES}
)

# SDL main workaround on some platforms
if(WIN32)
    target_link_libraries(tactile_browser_test SDL2main)
endif()

# Additional linker flags for different platforms
if(UNIX AND NOT APPLE)
    target_link_libraries(tactile_browser_test m pthread dl)
endif()

# MacOS specific flags
if(APPLE)
    target_link_libraries(tactile_browser_test "-framework Cocoa")
endif()
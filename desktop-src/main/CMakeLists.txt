cmake_minimum_required(VERSION 3.15)
project(tactilebrowser C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Wno-unused-parameter")

# Enable debugging symbols for development
set(CMAKE_BUILD_TYPE Debug)

# Find SDL2
find_package(SDL2 REQUIRED)

# Find libcurl
find_package(CURL REQUIRED)

# Platform-specific SDL2 handling
if(APPLE)
    # On macOS, SDL2 might be installed via Homebrew
    if(NOT SDL2_FOUND)
        find_library(SDL2_LIBRARY SDL2)
        find_path(SDL2_INCLUDE_DIR SDL2/SDL.h)
        set(SDL2_LIBRARIES ${SDL2_LIBRARY})
        set(SDL2_INCLUDE_DIRS ${SDL2_INCLUDE_DIR})
    endif()
endif()

# Check if subdirectories exist
if(NOT EXISTS ${CMAKE_SOURCE_DIR}/main/lvgl/CMakeLists.txt)
    message(FATAL_ERROR "LVGL subdirectory not found. Please ensure lvgl is properly added as a subdirectory.")
endif()

if(NOT EXISTS ${CMAKE_SOURCE_DIR}/main/lexbor/CMakeLists.txt)
    message(FATAL_ERROR "Lexbor subdirectory not found. Please ensure lexbor is properly added as a subdirectory.")
endif()

# Configure LVGL for SDL2 driver
set(LV_USE_SDL ON CACHE BOOL "Use SDL2 driver for LVGL")
set(LV_USE_DRAW_SDL ON CACHE BOOL "Use SDL2 for drawing")

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

# Set target properties
set_target_properties(tactile_browser_test PROPERTIES
    C_STANDARD 99
    C_STANDARD_REQUIRED ON
)

# Link libraries
target_link_libraries(tactile_browser_test
    lvgl
    lexbor_static
    ${SDL2_LIBRARIES}
    ${CURL_LIBRARIES}
)

# Platform-specific linking
if(WIN32)
    target_link_libraries(tactile_browser_test SDL2main)
elseif(UNIX AND NOT APPLE)
    target_link_libraries(tactile_browser_test m pthread dl)
elseif(APPLE)
    # macOS specific frameworks and libraries
    target_link_libraries(tactile_browser_test 
        "-framework Cocoa"
        "-framework IOKit"
        "-framework ForceFeedback"
        "-framework Carbon"
        "-framework CoreAudio"
        "-framework AudioToolbox"
        "-framework AVFoundation"
        "-framework CoreVideo"
        "-framework CoreGraphics"
        "-framework OpenGL"
    )
    
    # Set macOS deployment target
    set_target_properties(tactile_browser_test PROPERTIES
        MACOSX_DEPLOYMENT_TARGET "10.15"
    )
endif()

# Print configuration information
message(STATUS "SDL2 Libraries: ${SDL2_LIBRARIES}")
message(STATUS "SDL2 Include Dirs: ${SDL2_INCLUDE_DIRS}")
message(STATUS "CURL Libraries: ${CURL_LIBRARIES}")
message(STATUS "CURL Include Dirs: ${CURL_INCLUDE_DIRS}")
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")

# Optional: Create app bundle for macOS
if(APPLE)
    set_target_properties(tactile_browser_test PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.yourcompany.tactilebrowser"
        MACOSX_BUNDLE_BUNDLE_NAME "Tactile Browser"
        MACOSX_BUNDLE_BUNDLE_VERSION "1.0"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "1.0"
        MACOSX_BUNDLE_INFO_STRING "Tactile Browser - HTML parsing with LVGL"
    )
endif()
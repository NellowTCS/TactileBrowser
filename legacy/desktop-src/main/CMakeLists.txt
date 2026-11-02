cmake_minimum_required(VERSION 3.15)
project(tactilebrowser C CXX)

set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Wno-unused-parameter")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wno-unused-parameter")
# Don't set CMAKE_BUILD_TYPE here - let it be set from command line

# Allow user override of SDL2 paths (passed via -D)
if(NOT DEFINED SDL2_INCLUDE_DIR OR NOT DEFINED SDL2_LIBRARY)
    find_package(SDL2)
    if(NOT SDL2_FOUND)
        if(APPLE)
            find_library(SDL2_LIBRARY SDL2)
            find_path(SDL2_INCLUDE_DIR SDL2/SDL.h PATH_SUFFIXES SDL2)
            if(SDL2_LIBRARY AND SDL2_INCLUDE_DIR)
                set(SDL2_LIBRARIES ${SDL2_LIBRARY})
                get_filename_component(SDL2_INCLUDE_BASE "${SDL2_INCLUDE_DIR}" DIRECTORY)
                set(SDL2_INCLUDE_DIRS ${SDL2_INCLUDE_BASE})
                set(SDL2_FOUND TRUE)
            else()
                message(FATAL_ERROR "SDL2 not found. Please install SDL2 or specify SDL2_INCLUDE_DIR and SDL2_LIBRARY.")
            endif()
        else()
            message(FATAL_ERROR "SDL2 not found and no fallback available on this platform.")
        endif()
    else()
        set(SDL2_LIBRARIES SDL2::SDL2)
        set(SDL2_INCLUDE_DIRS ${SDL2_INCLUDE_DIR})
    endif()
else()
    set(SDL2_LIBRARIES ${SDL2_LIBRARY})
    set(SDL2_INCLUDE_DIRS ${SDL2_INCLUDE_DIR})
endif()

# libcurl
find_package(CURL REQUIRED)

# Check subdirs
if(NOT EXISTS ${CMAKE_SOURCE_DIR}/../fdm/CMakeLists.txt)
    message(FATAL_ERROR "Core library not found.")
endif()

# LVGL options
set(LV_USE_SDL ON CACHE BOOL "Use SDL2 driver for LVGL")
set(LV_USE_DRAW_SDL ON CACHE BOOL "Use SDL2 for drawing")
set(CONFIG_LV_USE_THORVG_INTERNAL OFF CACHE BOOL "Disable ThorVG internal")

# Set SDL2 cache variables for LVGL's find_package
set(SDL2_FOUND TRUE CACHE BOOL "" FORCE)
set(SDL2_INCLUDE_DIRS ${SDL2_INCLUDE_DIRS} CACHE STRING "" FORCE)
set(SDL2_LIBRARIES ${SDL2_LIBRARIES} CACHE STRING "" FORCE)
set(CMAKE_DISABLE_FIND_PACKAGE_SDL2 TRUE)

# Set CXX compiler to C compiler to avoid C++ issues with LVGL
# set(CMAKE_CXX_COMPILER ${CMAKE_C_COMPILER})  # Disabled - core library now uses C++

add_subdirectory(${CMAKE_SOURCE_DIR}/../fdm ${CMAKE_BINARY_DIR}/fdm EXCLUDE_FROM_ALL)

# Add LVGL subdirectory
add_subdirectory(${CMAKE_SOURCE_DIR}/../fdm/lvgl ${CMAKE_BINARY_DIR}/lvgl EXCLUDE_FROM_ALL)

# Add SDL2 include directories to LVGL target
target_include_directories(lvgl PRIVATE ${SDL2_INCLUDE_DIRS})

add_executable(TactileBrowser Source/main.cpp)

target_include_directories(TactileBrowser PRIVATE
    ${CURL_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}/../fdm/include
    ${CMAKE_SOURCE_DIR}/../fdm/lvgl
    ${CMAKE_SOURCE_DIR}/../fdm/lvgl/src
    ${CMAKE_SOURCE_DIR}/../fdm/lvgl/src/drivers/sdl
    ${SDL2_INCLUDE_DIRS}
)

target_link_libraries(TactileBrowser
    fdm
    lvgl
    ${SDL2_LIBRARIES}
    ${CURL_LIBRARIES}
    m
)

if(WIN32)
    target_link_libraries(TactileBrowser SDL2main)
elseif(UNIX AND NOT APPLE)
    target_link_libraries(TactileBrowser m pthread dl)
elseif(APPLE)
    target_link_libraries(TactileBrowser 
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
    set_target_properties(TactileBrowser PROPERTIES
        MACOSX_DEPLOYMENT_TARGET "10.15"
    )
endif()

# macOS Bundle
if(APPLE)
    set_target_properties(TactileBrowser PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.nellowtcs.tactilebrowser"
        MACOSX_BUNDLE_BUNDLE_NAME "Tactile Browser"
        MACOSX_BUNDLE_BUNDLE_VERSION "1.0"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "1.0"
        MACOSX_BUNDLE_INFO_STRING "Tactile Browser - HTML parsing with LVGL"
    )
endif()

message(STATUS "SDL2 Libraries: ${SDL2_LIBRARIES}")
message(STATUS "SDL2 Include Dirs: ${SDL2_INCLUDE_DIRS}")
message(STATUS "CURL Libraries: ${CURL_LIBRARIES}")
message(STATUS "CURL Include Dirs: ${CURL_INCLUDE_DIRS}")
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")

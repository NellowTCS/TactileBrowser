cmake_minimum_required(VERSION 3.15)
project(tactilebrowser C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Wno-unused-parameter")
set(CMAKE_BUILD_TYPE Debug)

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

# Check subdirs - use main/ prefix since that's where they are
if(NOT EXISTS ${CMAKE_SOURCE_DIR}/main/lvgl/CMakeLists.txt)
    message(FATAL_ERROR "LVGL subdirectory not found at ${CMAKE_SOURCE_DIR}/main/lvgl/")
endif()
if(NOT EXISTS ${CMAKE_SOURCE_DIR}/main/lexbor/CMakeLists.txt)
    message(FATAL_ERROR "Lexbor subdirectory not found at ${CMAKE_SOURCE_DIR}/main/lexbor/")
endif()

# LVGL options
set(LV_USE_SDL ON CACHE BOOL "Use SDL2 driver for LVGL")
set(LV_USE_DRAW_SDL ON CACHE BOOL "Use SDL2 for drawing")

# Add subdirectories with main/ prefix
add_subdirectory(main/lvgl EXCLUDE_FROM_ALL)
target_include_directories(lvgl PUBLIC ${SDL2_INCLUDE_DIRS})
target_link_libraries(lvgl PUBLIC ${SDL2_LIBRARIES})

add_subdirectory(main/lexbor)

# Main executable - use correct source path
add_executable(TactileBrowser main/Source/main.c)

target_include_directories(TactileBrowser PRIVATE
    ${CURL_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}/main/lvgl
    ${CMAKE_SOURCE_DIR}/main/lvgl/src
    ${CMAKE_SOURCE_DIR}/main/lvgl/src/drivers
    ${CMAKE_SOURCE_DIR}/main/lvgl/src/drivers/sdl
    ${CMAKE_SOURCE_DIR}/main/lexbor/include
    ${SDL2_INCLUDE_DIRS}
)

target_link_libraries(TactileBrowser
    lvgl
    lexbor_static
    ${SDL2_LIBRARIES}
    ${CURL_LIBRARIES}
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
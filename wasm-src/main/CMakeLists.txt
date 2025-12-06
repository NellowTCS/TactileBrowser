cmake_minimum_required(VERSION 3.15)
project(TactileBrowserWasm C CXX)

set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Wno-unused-parameter")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wno-unused-parameter")

# Emscripten-specific settings
set(CMAKE_EXECUTABLE_SUFFIX ".html")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -sUSE_SDL=2")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -sUSE_SDL=2")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -sUSE_SDL=2 -sALLOW_MEMORY_GROWTH=1 -sASYNCIFY -sEXPORT_ES6=1 -sMODULARIZE=1 -sEXPORTED_RUNTIME_METHODS=['ccall','cwrap'] -sEXPORTED_FUNCTIONS=['_main','_load_url','_render_html_from_js','_wasm_display_message','_start_app']")

# Add LVGL and core library
add_subdirectory(${CMAKE_SOURCE_DIR}/../tactilebrowser_core ${CMAKE_BINARY_DIR}/tactilebrowser_core EXCLUDE_FROM_ALL)
add_subdirectory(${CMAKE_SOURCE_DIR}/../tactilebrowser_core/lvgl ${CMAKE_BINARY_DIR}/lvgl EXCLUDE_FROM_ALL)

# LVGL include paths
target_include_directories(lvgl PRIVATE ${CMAKE_SOURCE_DIR}/../../tactilebrowser_core/lvgl ${CMAKE_SOURCE_DIR}/../../tactilebrowser_core/lvgl/src)

add_executable(TactileBrowserWasm Source/main.cpp)

target_include_directories(TactileBrowserWasm PRIVATE
    ${CMAKE_SOURCE_DIR}/../tactilebrowser_core/include
    ${CMAKE_SOURCE_DIR}/../tactilebrowser_core/lvgl
    ${CMAKE_SOURCE_DIR}/../tactilebrowser_core/lvgl/src
    ${CMAKE_SOURCE_DIR}/../tactilebrowser_core/lvgl/src/drivers/sdl
)

target_link_libraries(TactileBrowserWasm
    tactilebrowser_core
    lvgl
    m
)

set_target_properties(TactileBrowserWasm PROPERTIES LINK_FLAGS "-sUSE_SDL=2 -sASYNCIFY -sALLOW_MEMORY_GROWTH=1")
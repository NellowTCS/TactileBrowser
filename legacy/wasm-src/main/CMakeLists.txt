cmake_minimum_required(VERSION 3.15)
project(TactileBrowserWasm C CXX)

set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Wno-unused-parameter")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wno-unused-parameter")

# Emscripten-specific settings
set(CMAKE_EXECUTABLE_SUFFIX ".html")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -sALLOW_MEMORY_GROWTH=1 -sASYNCIFY -sEXPORT_ES6=1 -sMODULARIZE=1 -sEXPORTED_RUNTIME_METHODS=['ccall','cwrap'] -sEXPORTED_FUNCTIONS=['_main','_start_app','_wasm_render_html','_wasm_pointer_event','_wasm_scroll_event']")

# Add fdm core library
add_subdirectory(${CMAKE_SOURCE_DIR}/../fdm ${CMAKE_BINARY_DIR}/fdm EXCLUDE_FROM_ALL)

add_executable(TactileBrowserWasm Source/main.cpp)

target_include_directories(TactileBrowserWasm PRIVATE
    ${CMAKE_SOURCE_DIR}/../fdm/include
    ${CMAKE_SOURCE_DIR}/../fdm/lexbor/source
)

target_link_libraries(TactileBrowserWasm
    fdm
    m
)

set_target_properties(TactileBrowserWasm PROPERTIES LINK_FLAGS "-sASYNCIFY -sALLOW_MEMORY_GROWTH=1")

cmake_minimum_required(VERSION 3.16)
project(TactileBrowserCoreTests LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(${CMAKE_SOURCE_DIR}/../tactilebrowser_core ${CMAKE_BINARY_DIR}/tactilebrowser_core EXCLUDE_FROM_ALL)

add_executable(tactilebrowser_core_tests
    src/layout_render_tests.cpp
    src/TestRenderer.cpp
)

target_include_directories(tactilebrowser_core_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/../tactilebrowser_core/include
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(tactilebrowser_core_tests PRIVATE tactilebrowser_core)

enable_testing()
add_test(NAME tactilebrowser_core_tests COMMAND tactilebrowser_core_tests)

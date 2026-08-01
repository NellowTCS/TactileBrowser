cmake_minimum_required(VERSION 3.16)
project(FdmCoreTests LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(${CMAKE_SOURCE_DIR}/../fdm ${CMAKE_BINARY_DIR}/fdm EXCLUDE_FROM_ALL)

add_executable(fdm_core_tests
    src/layout_render_tests.cpp
    src/TestRenderer.cpp
)

target_include_directories(fdm_core_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/../fdm/include
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(fdm_core_tests PRIVATE fdm)

enable_testing()
add_test(NAME fdm_core_tests COMMAND fdm_core_tests)

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

add_executable(fdm_reftest
    src/reftest_runner.cpp
    src/find_ref.cpp
    src/dump_utils.cpp
    src/TestRenderer.cpp
)

target_include_directories(fdm_reftest PRIVATE
    ${CMAKE_SOURCE_DIR}/../fdm/include
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(fdm_reftest PRIVATE fdm)

# Internal hand-written reftest/snapshot cases.
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/reftests)
    add_test(NAME fdm_reftest_cases COMMAND fdm_reftest
        --dir ${CMAKE_CURRENT_SOURCE_DIR}/reftests/cases
        --skip ${CMAKE_CURRENT_SOURCE_DIR}/reftests/.skip
    )

    # Vendored web-platform-tests reftest subsets
    if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/wpt/css/CSS2)
        add_test(NAME fdm_reftest_css2 COMMAND fdm_reftest
            --dir ${CMAKE_CURRENT_SOURCE_DIR}/wpt/css/CSS2
            --skip ${CMAKE_CURRENT_SOURCE_DIR}/reftests/.skip
            --xfail ${CMAKE_CURRENT_SOURCE_DIR}/reftests/wpt_css2.xfail
        )
    endif()
    if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/wpt/css/css-backgrounds)
        add_test(NAME fdm_reftest_css_backgrounds COMMAND fdm_reftest
            --dir ${CMAKE_CURRENT_SOURCE_DIR}/wpt/css/css-backgrounds
            --skip ${CMAKE_CURRENT_SOURCE_DIR}/reftests/.skip
            --xfail ${CMAKE_CURRENT_SOURCE_DIR}/reftests/wpt_css_backgrounds.xfail
        )
    endif()
endif()

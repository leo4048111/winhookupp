cmake_minimum_required(VERSION 3.22)

project(winhookupp)

# -- project dirs --
set(WINHOOKUPP_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(WINHOOKUPP_SRC_DIR ${WINHOOKUPP_ROOT_DIR}/src)
set(WINHOOKUPP_TESTS_DIR ${WINHOOKUPP_ROOT_DIR}/unit-tests)
set(WINHOOKUPP_INCLUDE_DIR ${WINHOOKUPP_ROOT_DIR}/include)

## --project configs --
set(BUILD_TESTS ON CACHE BOOL "Build test programs")
set(BUILD_EXAMPLE ON CACHE BOOL "Build example program")

# -- deps
add_subdirectory(${WINHOOKUPP_ROOT_DIR}/third-party/googletest)

set(WINHOOKUPP_SRC
    ${WINHOOKUPP_SRC_DIR}/veh.cpp
    ${WINHOOKUPP_SRC_DIR}/trampoline.cpp
    ${WINHOOKUPP_SRC_DIR}/memory.cpp
    ${WINHOOKUPP_SRC_DIR}/hde/hde32.c
    ${WINHOOKUPP_SRC_DIR}/hde/hde64.c
)

set(WINHOOKUPP_TESTS
    ${WINHOOKUPP_TESTS_DIR}/winhookupp_test_main.cpp
    ${WINHOOKUPP_TESTS_DIR}/winhookupp_test_memory.cpp
    ${WINHOOKUPP_TESTS_DIR}/winhookupp_test_veh.cpp
    ${WINHOOKUPP_TESTS_DIR}/winhookupp_test_instructions.cpp
    ${WINHOOKUPP_TESTS_DIR}/winhookupp_test_trampoline.cpp
    )

include_directories(${WINHOOKUPP_INCLUDE_DIR} ${GOOGLETEST_INCLUDE_DIR})

add_library(winhookupp STATIC ${WINHOOKUPP_SRC})

if(BUILD_TESTS)
    message("Enabled tests")
    add_executable(winhookupp_test ${WINHOOKUPP_TESTS})
    target_link_libraries(winhookupp_test winhookupp gtest)
else()
    message("Disabled tests")
endif()
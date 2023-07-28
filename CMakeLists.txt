cmake_minimum_required(VERSION 3.22)

project(winhookupp)

set(WINHOOKUPP_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(WINHOOKUPP_SRC_DIR ${WINHOOKUPP_ROOT_DIR}/src)
set(WINHOOKUPP_INCLUDE_DIR ${WINHOOKUPP_ROOT_DIR}/include)

set(WINHOOKUPP_SRC_FILES
    ${WINHOOKUPP_SRC_DIR}/veh.cpp
    ${WINHOOKUPP_SRC_DIR}/trampoline.cpp
    ${WINHOOKUPP_SRC_DIR}/memory.cpp
    ${WINHOOKUPP_SRC_DIR}/hde/hde32.c
    ${WINHOOKUPP_SRC_DIR}/hde/hde64.c
)

include_directories(${WINHOOKUPP_INCLUDE_DIR})

add_executable(winhookupp ${WINHOOKUPP_SRC_FILES} main.cpp)
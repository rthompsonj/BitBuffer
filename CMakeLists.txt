cmake_minimum_required(VERSION 2.6)
project(bitbuffer C)

set(BITBUFFER_STATIC "0" CACHE BOOL "Create a static library")
set(BITBUFFER_SHARED "0" CACHE BOOL "Create a shared library")
set(BITBUFFER_TEST "0" CACHE BOOL "Create a test executable")

include_directories(${PROJECT_SOURCE_DIR}/Source)

if (BITBUFFER_STATIC)
    add_library(bitbuffer_static STATIC Source/bitbuffer.c)
    target_link_libraries(bitbuffer_static)
endif()

if (BITBUFFER_SHARED)
    add_library(bitbuffer SHARED Source/bitbuffer.c)
    target_link_libraries(bitbuffer)
endif()

if (BITBUFFER_TEST)
	include_directories(${CMAKE_CURRENT_SOURCE_DIR}/Source)
	add_executable(bitbuffer-test ${CMAKE_CURRENT_SOURCE_DIR}/Test/test.c)
	target_link_libraries(bitbuffer-test bitbuffer)
endif()
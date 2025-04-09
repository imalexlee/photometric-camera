cmake_minimum_required(VERSION 3.30)
project(photometric_camera)
include(FetchContent)

set(CMAKE_CXX_STANDARD 20)

file(GLOB_RECURSE project_sources "src/*.cpp")
add_executable(${CMAKE_PROJECT_NAME} ${project_sources})

FetchContent_Declare(
        vk-lib
        GIT_REPOSITORY https://github.com/imalexlee/vk-lib.git
        GIT_TAG acbb9b7275864ab84f52f7c1f2889dced77bb4e3
)

FetchContent_Declare(
        GLFW
        URL https://github.com/glfw/glfw/releases/download/3.4/glfw-3.4.zip
)

FetchContent_MakeAvailable(vk-lib)
FetchContent_MakeAvailable(GLFW)

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

target_link_libraries(${CMAKE_PROJECT_NAME} PUBLIC vk-lib glfw)

target_include_directories(${CMAKE_PROJECT_NAME} PUBLIC src vendor)
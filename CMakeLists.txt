cmake_minimum_required(VERSION 3.30)
project(photometric_camera)
include(FetchContent)

set(CMAKE_CXX_STANDARD 20)

add_executable(${CMAKE_PROJECT_NAME} src/main.cpp src/vk_context.cpp src/swapchain.cpp src/renderer.cpp src/frame.cpp src/vma_usage.cpp)

FetchContent_Declare(
        vk-lib
        GIT_REPOSITORY https://github.com/imalexlee/vk-lib.git
        GIT_TAG b881550726c73f8e2aff21e5d10b0e96270352ba
)

FetchContent_MakeAvailable(vk-lib)

FetchContent_Declare(
        vma
        URL https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator/archive/refs/tags/v3.2.1.zip
)
FetchContent_MakeAvailable(vma)

FetchContent_Declare(
        GLFW
        URL https://github.com/glfw/glfw/releases/download/3.4/glfw-3.4.zip
)

FetchContent_MakeAvailable(GLFW)

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

target_link_libraries(${CMAKE_PROJECT_NAME} PUBLIC vk-lib glfw GPUOpen::VulkanMemoryAllocator)

target_include_directories(${CMAKE_PROJECT_NAME} PUBLIC src)
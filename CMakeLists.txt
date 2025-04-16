cmake_minimum_required(VERSION 3.29)
project(photometric_camera)
include(FetchContent)

set(CMAKE_CXX_STANDARD 20)

file(GLOB_RECURSE project_sources "src/*.cpp")
add_executable(${CMAKE_PROJECT_NAME} ${project_sources})

FetchContent_Declare(
        vk-lib
        GIT_REPOSITORY https://github.com/imalexlee/vk-lib.git
        GIT_TAG ca669255f6c2d6047baeced9df3138b1badf09a0
)

set(GLFW_BUILD_DOCS OFF CACHE BOOL "Disable GLFW docs" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "Disable GLFW tests" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "Disable GLFW examples" FORCE)
FetchContent_Declare(
        GLFW
        URL https://github.com/glfw/glfw/releases/download/3.4/glfw-3.4.zip
)

set(KTX_FEATURE_TESTS OFF CACHE BOOL "Disable KTX tests" FORCE)
set(KTX_FEATURE_TOOLS OFF CACHE BOOL "Disable KTX tools" FORCE)
set(KTX_FEATURE_TOOLS_CTS OFF CACHE BOOL "Disable KTX CTS tests" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build static libraries" FORCE)
FetchContent_Declare(
        ktx
        GIT_REPOSITORY https://github.com/KhronosGroup/KTX-Software.git
        GIT_TAG beef80159525d9fb7abb8645ea85f4c4f6842e8f
        GIT_SUBMODULES ""
)

FetchContent_Declare(
        glm
        GIT_REPOSITORY https://github.com/g-truc/glm.git
        GIT_TAG bf71a834948186f4097caa076cd2663c69a10e1e #refs/tags/1.0.1
)

FetchContent_MakeAvailable(vk-lib)
FetchContent_MakeAvailable(GLFW)
FetchContent_MakeAvailable(ktx)
FetchContent_MakeAvailable(glm)

target_link_libraries(${CMAKE_PROJECT_NAME} PUBLIC vk-lib glfw ktx glm::glm)

target_include_directories(${CMAKE_PROJECT_NAME} PUBLIC src vendor)
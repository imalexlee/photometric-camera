cmake_minimum_required(VERSION 3.29)
project(photometric_camera)
include(FetchContent)

set(CMAKE_CXX_STANDARD 20)

find_package(Vulkan REQUIRED)

file(GLOB_RECURSE project_sources "src/*.cpp")
add_executable(${CMAKE_PROJECT_NAME} ${project_sources})

FetchContent_Declare(
        vk-gltf
        GIT_REPOSITORY https://github.com/imalexlee/vk-gltf.git
        GIT_TAG 272ba5579bea03152df9db350a4af9354b69bb9c
)

FetchContent_Declare(
        vk-lib
        GIT_REPOSITORY https://github.com/imalexlee/vk-lib.git
        GIT_TAG 8d7030ee258442d974f3d9905f8a919b68726712
)


set(GLFW_BUILD_DOCS OFF CACHE BOOL "Disable GLFW docs" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "Disable GLFW tests" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "Disable GLFW examples" FORCE)
FetchContent_Declare(
        GLFW
        URL https://github.com/glfw/glfw/releases/download/3.4/glfw-3.4.zip
)

FetchContent_Declare(
        glm
        URL https://github.com/g-truc/glm/archive/refs/tags/1.0.1.zip
)

FetchContent_MakeAvailable(vk-lib vk-gltf GLFW glm)

target_link_libraries(${CMAKE_PROJECT_NAME} PUBLIC vk-lib vk-gltf glfw glm::glm)

target_include_directories(${CMAKE_PROJECT_NAME} PUBLIC src vendor)
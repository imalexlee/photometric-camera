cmake_minimum_required(VERSION 3.29)
project(photometric_camera)
include(FetchContent)

set(CMAKE_CXX_STANDARD 20)

find_package(Vulkan REQUIRED)

file(GLOB_RECURSE project_sources "src/*.cpp")
add_executable(${CMAKE_PROJECT_NAME} ${project_sources})

set(VK_GLTF_USE_VOLK_OPT ON)
FetchContent_Declare(
        vk-gltf
        GIT_REPOSITORY https://github.com/imalexlee/vk-gltf.git
        GIT_TAG 898d5f8486f0aad4112f48abc22e515acbc01c2a
)

FetchContent_Declare(
        volk
        URL https://github.com/zeux/volk/archive/refs/tags/1.4.304.zip
)

FetchContent_Declare(
        vk-lib
        GIT_REPOSITORY https://github.com/imalexlee/vk-lib.git
        GIT_TAG 1d03e2bc32f51d072856f71c59ff88b62d4fa5bd
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

FetchContent_MakeAvailable(vk-lib vk-gltf GLFW glm volk)

target_link_libraries(${CMAKE_PROJECT_NAME} PUBLIC vk-lib vk-gltf glfw glm::glm volk)

target_include_directories(${CMAKE_PROJECT_NAME} PUBLIC src vendor)
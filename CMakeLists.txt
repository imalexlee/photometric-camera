cmake_minimum_required(VERSION 3.30)
project(photometric_camera)
include(FetchContent)

set(CMAKE_CXX_STANDARD 20)

add_executable(${CMAKE_PROJECT_NAME} src/main.cpp)

FetchContent_Declare(
       vk-lib
        GIT_REPOSITORY https://github.com/imalexlee/vk-lib.git
        GIT_TAG b881550726c73f8e2aff21e5d10b0e96270352ba
)

FetchContent_MakeAvailable(vk-lib)

target_link_libraries(${CMAKE_PROJECT_NAME} PUBLIC vk-lib)
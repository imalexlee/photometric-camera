#include "common.h"

#include "loader.h"
#include "renderer.h"

int main() {

    LoadOptions gltf_load_options{};
    gltf_load_options.gltf_path = "../assets/DamagedHelmet.glb";
    gltf_load_options.cache_dir = "cache/";
    load_gltf(&gltf_load_options);
    // Renderer renderer = renderer_create();
    //
    // while (!glfwWindowShouldClose(renderer.window)) {
    //     glfwPollEvents();
    //     int width, height;
    //     glfwGetFramebufferSize(renderer.window, &width, &height);
    //     while (width == 0 || height == 0) {
    //         glfwGetFramebufferSize(renderer.window, &width, &height);
    //         glfwWaitEvents();
    //     }
    //     renderer_draw(&renderer);
    // }
}
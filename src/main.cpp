#include "common.h"

#include "loader.h"
#include "renderer.h"

int main() {

    load_gltf("../assets/DamagedHelmet.glb");
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
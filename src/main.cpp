#include "common.h"

#include "loader.h"
#include "renderer.h"

int main() {

    Renderer renderer = renderer_create();

    LoadOptions gltf_load_options{};
    gltf_load_options.gltf_path = "../assets/DamagedHelmet.glb";
    gltf_load_options.cache_dir = "cache/";
    load_gltf(&gltf_load_options, renderer.vk_context.device, renderer.allocator, renderer.vk_context.frame_command_pool,
              renderer.vk_context.graphics_queue, renderer.vk_context.queue_family);

    while (!glfwWindowShouldClose(renderer.window)) {
        glfwPollEvents();
        int width, height;
        glfwGetFramebufferSize(renderer.window, &width, &height);
        while (width == 0 || height == 0) {
            glfwGetFramebufferSize(renderer.window, &width, &height);
            glfwWaitEvents();
        }
        renderer_draw(&renderer);
    }
}
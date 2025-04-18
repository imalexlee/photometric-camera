#include "common.h"

#include "loader.h"
#include "renderer.h"

int main() {

    Renderer renderer = renderer_create();

    while (!glfwWindowShouldClose(renderer.window.glfw_window)) {
        glfwPollEvents();
        int width, height;
        glfwGetFramebufferSize(renderer.window.glfw_window, &width, &height);
        while (width == 0 || height == 0) {
            glfwGetFramebufferSize(renderer.window.glfw_window, &width, &height);
            glfwWaitEvents();
        }
        renderer_draw(&renderer);
    }
}
#include "window.h"

#include <camera.h>
#include <renderer.h>

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);

Window window_create() {
    if (!glfwInit()) {
        abort_message("GLFW cannot be initialized");
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    Window window{};

    window.glfw_window = glfwCreateWindow(1920, 1080, "Photometric Camera", nullptr, nullptr);

    window_register_key_callback(&window, camera_key_callback);
    window_register_key_callback(&window, renderer_key_callback);

    glfwSetKeyCallback(window.glfw_window, key_callback);

    glfwSetCursorPosCallback(window.glfw_window, camera_cursor_callback);
    glfwSetMouseButtonCallback(window.glfw_window, camera_mouse_button_callback);

    if (glfwRawMouseMotionSupported()) {
        glfwSetInputMode(window.glfw_window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
    }

    return window;
}

void window_register_key_callback(Window* window, std::function<void(GLFWwindow*, int, int, int, int)>&& fn_ptr) {
    Window::key_callbacks.push_back(fn_ptr);
}

static void key_callback(GLFWwindow* window, int key, [[maybe_unused]] int scancode, int action, [[maybe_unused]] int mods) {

    if (action == GLFW_PRESS) {
        if (key == GLFW_KEY_ESCAPE) {
            glfwSetWindowShouldClose(window, GLFW_TRUE);
        }
    }

    for (auto& callback : Window::key_callbacks) {
        callback(window, key, scancode, action, mods);
    }
}
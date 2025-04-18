#include "window.h"

#include <camera.h>

Window window_create() {
    if (!glfwInit()) {
        abort_message("GLFW cannot be initialized");
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    Window window{};

    window.glfw_window = glfwCreateWindow(1920, 1080, "Photometric Camera", nullptr, nullptr);

    glfwSetKeyCallback(window.glfw_window, camera_key_callback);
    glfwSetCursorPosCallback(window.glfw_window, camera_cursor_callback);
    glfwSetMouseButtonCallback(window.glfw_window, camera_mouse_button_callback);

    glfwSetInputMode(window.glfw_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    if (glfwRawMouseMotionSupported()) {
        glfwSetInputMode(window.glfw_window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
    }

    return window;
}

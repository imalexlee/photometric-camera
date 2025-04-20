#pragma once
#include "common.h"

struct Window {
    GLFWwindow* glfw_window{};

    static inline std::vector<std::function<void(GLFWwindow*, int key, int scancode, int action, int mods)>> key_callbacks;
};

[[nodiscard]] Window window_create();

void window_register_key_callback(Window* window, std::function<void(GLFWwindow*, int, int, int, int)>&& fn_ptr);
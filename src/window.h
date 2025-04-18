#pragma once
#include "common.h"

struct Window {
    GLFWwindow* glfw_window{};
};

[[nodiscard]] Window window_create();

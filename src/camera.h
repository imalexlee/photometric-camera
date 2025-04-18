#pragma once
#include "common.h"

struct Camera {
    glm::vec<2, double> cursor_pos{};
    glm::vec3           look_at{};
    glm::vec3           eye_pos{0, 0, 2};
    glm::vec3           velocity{};
    float               yaw{};
    float               pitch{};
    bool                movement_enabled{true};
};

namespace global {
inline Camera camera{};
} // namespace global

void camera_key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);

void camera_cursor_callback(GLFWwindow* window, double x_pos, double y_pos);

void camera_mouse_button_callback(GLFWwindow* window, int button, int action, int mods);

[[nodiscard]] glm::mat4 camera_rotation();

[[nodiscard]] glm::mat4 camera_view();

void camera_update(float delta_time);

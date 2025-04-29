#include "camera.h"

static constexpr float movement_speed = 12.f;

void camera_key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (!global::camera.movement_enabled) {
        global::camera.velocity = {0, 0, 0};
        return;
    }
    if (key == GLFW_KEY_W) {
        if (action == GLFW_PRESS) {
            if (mods == GLFW_MOD_SHIFT) {

                global::camera.velocity.y = -movement_speed;
            } else {
                global::camera.velocity.z = -movement_speed;
            }
        }
        if (action == GLFW_RELEASE) {
            global::camera.velocity.z = 0;
            global::camera.velocity.y = 0;
        }
    }

    if (key == GLFW_KEY_S) {
        if (action == GLFW_PRESS) {
            if (mods == GLFW_MOD_SHIFT) {
                global::camera.velocity.y = movement_speed;
            } else {
                global::camera.velocity.z = movement_speed;
            }
        }
        if (action == GLFW_RELEASE) {
            global::camera.velocity.z = 0;
            global::camera.velocity.y = 0;
        }
    }

    if (key == GLFW_KEY_A) {
        if (action == GLFW_PRESS) {
            global::camera.velocity.x = -movement_speed;
        }
        if (action == GLFW_RELEASE) {
            global::camera.velocity.x = 0;
        }
    }

    if (key == GLFW_KEY_D) {
        if (action == GLFW_PRESS) {
            global::camera.velocity.x = movement_speed;
        }
        if (action == GLFW_RELEASE) {
            global::camera.velocity.x = 0;
        }
    }
}

void camera_cursor_callback(GLFWwindow* window, double x_pos, double y_pos) {
    if (!global::camera.movement_enabled) {
        global::camera.cursor_pos = {x_pos, y_pos};
        return;
    }

    float x_relative = global::camera.cursor_pos.x - x_pos;
    float y_relative = global::camera.cursor_pos.y - y_pos;

    global::camera.yaw -= x_relative / 1000.f;
    global::camera.pitch += y_relative / 1000.f;

    global::camera.cursor_pos = {x_pos, y_pos};
}

void camera_mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        global::camera.movement_enabled = !global::camera.movement_enabled;
        if (global::camera.movement_enabled) {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        } else {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        }
    }
}

glm::mat4 camera_rotation() {
    glm::quat pitch_rot = glm::angleAxis(global::camera.pitch, glm::vec3{1, 0, 0});
    glm::quat yaw_rot   = glm::angleAxis(global::camera.yaw, glm::vec3{0, -1, 0});
    return glm::toMat4(yaw_rot) * glm::toMat4(pitch_rot);
}

glm::mat4 camera_view() {
    glm::mat4 translation = glm::translate(glm::mat4(1.f), global::camera.eye_pos);
    return glm::inverse(translation * camera_rotation());
}

void set_camera_proj(float fov_y_radians, float aspect_ratio) {
    glm::mat4 proj = glm::perspective(fov_y_radians, aspect_ratio, 10000.f, 0.01f);
    proj[1][1] *= -1;

    global::camera.proj = proj;
}

void camera_update(float delta_time) {
    if (global::camera.velocity == glm::vec3{0, 0, 0}) {
        return;
    }
    global::camera.eye_pos += glm::vec3(camera_rotation() * glm::vec4(global::camera.velocity * delta_time, 0));
}
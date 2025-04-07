#pragma once
#include "common.h"

// #define GLFW_INCLUDE_VULKAN
// #include <GLFW/glfw3.h>

struct VkContext {
    VkInstance       instance{};
    VkPhysicalDevice physical_device{};
    VkDevice         device{};
    VkCommandPool    frame_command_pool{};
    VkQueue          graphics_queue{};
    VkQueue          present_queue{};
    uint32_t         queue_family{};
    VkSurfaceKHR     surface{};
    //    GraphicsPipeline   graphics_pipeline{};
    uint64_t curr_frame{};
};

[[nodiscard]] VkContext vk_context_create(GLFWwindow* window);

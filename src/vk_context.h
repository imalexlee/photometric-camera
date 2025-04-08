#pragma once
#include "common.h"

struct VkContext {
    VkInstance       instance{};
    VkPhysicalDevice physical_device{};
    VkDevice         device{};
    VkCommandPool    frame_command_pool{};
    VkQueue          graphics_queue{};
    VkQueue          present_queue{};
    uint32_t         queue_family{};
    VkSurfaceKHR     surface{};
};

[[nodiscard]] VkContext vk_context_create(GLFWwindow* window);

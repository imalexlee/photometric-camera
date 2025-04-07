#pragma once
#include "common.h"

struct SwapchainContext {
    VkSwapchainKHR           swapchain{};
    VkSurfaceFormatKHR       surface_format{};
    VkExtent2D               extent{};
    std::vector<VkImage>     images{};
    std::vector<VkImageView> image_views{};
};

[[nodiscard]] SwapchainContext swapchain_context_create(VkPhysicalDevice physical_device, VkDevice device, VkSurfaceKHR surface, GLFWwindow* window);

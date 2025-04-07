#pragma once
#include "common.h"

#include <frame.h>
#include <swapchain.h>
#include <vk_context.h>

struct Renderer {
    VkContext          vk_context{};
    SwapchainContext   swapchain_context{};
    std::vector<Frame> frames{};
    GLFWwindow*        window{};
};

Renderer renderer_create();

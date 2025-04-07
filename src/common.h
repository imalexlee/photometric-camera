#pragma once

#include <vk_lib/commands.h>
#include <vk_lib/common.h>
#include <vk_lib/core.h>
#include <vk_lib/presentation.h>
#include <vk_lib/rendering.h>
#include <vk_lib/resources.h>
#include <vk_lib/synchronization.h>
#include <vulkan/vk_enum_string_helper.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <algorithm>

#define VK_CHECK(x)                                                                                                                                  \
    do {                                                                                                                                             \
        VkResult err = x;                                                                                                                            \
        if (err) {                                                                                                                                   \
            std::cerr << "Detected Vulkan error: " << string_VkResult(err) << std::endl;                                                             \
            abort();                                                                                                                                 \
        }                                                                                                                                            \
    } while (0)

[[noreturn]] inline void abort_message(std::string_view message) {
    std::cerr << message << std::endl;
    std::abort();
}
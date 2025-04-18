#pragma once

#include <vk_lib/commands.h>
#include <vk_lib/core.h>
#include <vk_lib/pipelines.h>
#include <vk_lib/presentation.h>
#include <vk_lib/rendering.h>
#include <vk_lib/resources.h>
#include <vk_lib/shader_data.h>
#include <vk_lib/shaders.h>
#include <vk_lib/synchronization.h>
#include <vulkan/vk_enum_string_helper.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <algorithm>
#include <array>
#include <filesystem>
#include <fstream>

#define VMA_STATIC_VULKAN_FUNCTIONS 0
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 0
#include <vk_mem_alloc.h>

#define GLM_FORCE_INTRINSICS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL

#include <glm/ext.hpp>
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>

#include <cgltf.h>

#define VK_CHECK(x)                                                                                                                                  \
    do {                                                                                                                                             \
        VkResult err = x;                                                                                                                            \
        if (err) {                                                                                                                                   \
            std::cerr << "Detected Vulkan error: " << string_VkResult(err) << std::endl;                                                             \
            abort();                                                                                                                                 \
        }                                                                                                                                            \
    } while (0)

[[noreturn]] inline void abort_message(const std::string_view message) {
    std::cerr << message << std::endl;
    std::abort();
}

struct AllocatedImage {
    VkImage           image{};
    VkImageView       image_view{};
    VkFormat          image_format{};
    VkImageLayout     layout{};
    VkExtent3D        extent{};
    VmaAllocation     allocation{};
    VmaAllocationInfo allocation_info{};
};

struct AllocatedBuffer {
    VkBuffer          buffer{};
    VkDeviceAddress   address{};
    VkBufferView      buffer_view{};
    VmaAllocation     allocation{};
    VmaAllocationInfo allocation_info{};
};

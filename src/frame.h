#pragma once

#include "common.h"

struct Frame {
    VkSemaphore        image_available_semaphore{};
    VkSemaphore        render_finished_semaphore{};
    VkFence            in_flight_fence{};
    VkRenderingInfoKHR rendering_info{};
    VkCommandBuffer    command_buffer{};
};

std::vector<Frame> frames_create(VkDevice device, VkCommandPool command_pool, std::span<const VkImageView> frame_image_views,
                                 std::span<const VkImage> frame_images, uint32_t queue_family_index, VkImageView msaa_image_view);

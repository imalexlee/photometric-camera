#pragma once

#include "common.h"

struct Frame {
    VkSemaphore                  image_available_semaphore{};
    VkSemaphore                  render_finished_semaphore{};
    VkFence                      in_flight_fence{};
    VkRenderingInfoKHR           rendering_info{};
    VkCommandBuffer              command_buffer{};
    VkRenderingAttachmentInfoKHR attachment_info{};
    VkCommandBufferSubmitInfoKHR command_buffer_submit_info{};
    VkSemaphoreSubmitInfoKHR     wait_semaphore_submit_info{};
    VkSemaphoreSubmitInfoKHR     signal_semaphore_submit_info{};
    VkSubmitInfo2                submit_info_2{};
    VkImageMemoryBarrier2        present_image_memory_barrier{};
    VkDependencyInfoKHR          present_dependency_info{};
    VkImageMemoryBarrier2        draw_image_memory_barrier{};
    VkDependencyInfoKHR          draw_dependency_info{};
};

std::vector<Frame> frames_create(VkDevice device, VkCommandPool command_pool, std::span<const VkImageView> frame_image_views,
                                 std::span<const VkImage> frame_images, uint32_t queue_family_index, VkImageView msaa_image_view);

#include "frame.h"

std::vector<Frame> frames_create(VkDevice device, VkCommandPool command_pool, std::span<const VkImageView> frame_image_views,
                                 std::span<const VkImage> frame_images, uint32_t queue_family_index, VkImageView msaa_image_view) {
    std::vector<Frame> frames;
    frames.resize(frame_image_views.size());

    for (uint32_t i = 0; i < frame_image_views.size(); i++) {
        Frame* frame = &frames[i];

        VkCommandBufferAllocateInfo command_buffer_ai = vk_lib::command_buffer_allocate_info(command_pool);
        vkAllocateCommandBuffers(device, &command_buffer_ai, &frame->command_buffer);

        VkSemaphoreCreateInfo semaphore_ci = vk_lib::semaphore_create_info();
        VK_CHECK(vkCreateSemaphore(device, &semaphore_ci, nullptr, &frame->image_available_semaphore));
        VK_CHECK(vkCreateSemaphore(device, &semaphore_ci, nullptr, &frame->render_finished_semaphore));

        VkFenceCreateInfo fence_ci = vk_lib::fence_create_info(VK_FENCE_CREATE_SIGNALED_BIT);
        VK_CHECK(vkCreateFence(device, &fence_ci, nullptr, &frame->in_flight_fence));
    }

    return frames;
}

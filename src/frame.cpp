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

        frame->command_buffer_submit_info = vk_lib::command_buffer_submit_info(frame->command_buffer);
        frame->wait_semaphore_submit_info =
            vk_lib::semaphore_submit_info(frame->image_available_semaphore, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR);
        frame->signal_semaphore_submit_info =
            vk_lib::semaphore_submit_info(frame->render_finished_semaphore, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR);

        frame->submit_info_2 =
            vk_lib::submit_info_2(&frame->command_buffer_submit_info, &frame->wait_semaphore_submit_info, &frame->signal_semaphore_submit_info);

        const VkImageSubresourceRange subresource_range = vk_lib::image_subresource_range(VK_IMAGE_ASPECT_COLOR_BIT);

        VkImage     frame_image      = frame_images[i];
        VkImageView frame_image_view = frame_image_views[i];

        VkClearValue clear_value{};
        clear_value.color      = {0, 0, 0, 0};
        frame->attachment_info = vk_lib::rendering_attachment_info(
            msaa_image_view, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_DONT_CARE, &clear_value,
            VK_RESOLVE_MODE_AVERAGE_BIT, frame_image_view, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

        frame->draw_image_memory_barrier = vk_lib::image_memory_barrier_2(
            frame_image, subresource_range, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, queue_family_index, queue_family_index,
            VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR,
            VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_2_NONE);

        frame->draw_dependency_info = vk_lib::dependency_info(&frame->draw_image_memory_barrier, nullptr, nullptr);

        frame->present_image_memory_barrier = vk_lib::image_memory_barrier_2(
            frame_image, subresource_range, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, queue_family_index, queue_family_index,
            VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR,
            VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_2_NONE);

        frame->present_dependency_info = vk_lib::dependency_info(&frame->present_image_memory_barrier, nullptr, nullptr);
    }

    return frames;
}

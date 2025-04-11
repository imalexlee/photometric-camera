#include "renderer.h"

static VkShaderModule load_shader(VkDevice device, const std::filesystem::path& path) {
    std::ifstream file(path, std::ios::ate | std::ios::binary);
    if (!file.is_open()) {
        abort_message("Failed to find shader");
    }
    const size_t      file_size = file.tellg();
    std::vector<char> shader_data(file_size);
    file.seekg(0);
    file.read(shader_data.data(), static_cast<uint32_t>(file_size));
    VkShaderModule           shader_module;
    VkShaderModuleCreateInfo shader_module_ci = vk_lib::shader_module_create_info(reinterpret_cast<const uint32_t*>(shader_data.data()), file_size);
    VK_CHECK(vkCreateShaderModule(device, &shader_module_ci, nullptr, &shader_module));
    return shader_module;
}
static GraphicsPipeline create_graphics_pipeline(VkDevice device, VkFormat color_attachment_format, uint32_t width, uint32_t height) {

    VkPipelineLayoutCreateInfo layout_create_info = vk_lib::pipeline_layout_create_info();
    VkPipelineLayout           pipeline_layout;
    vkCreatePipelineLayout(device, &layout_create_info, nullptr, &pipeline_layout);

    std::array                             color_attachment_formats = {color_attachment_format};
    const VkPipelineRenderingCreateInfoKHR rendering_create_info    = vk_lib::pipeline_rendering_create_info(color_attachment_formats);

    VkShaderModule                         vert_shader        = load_shader(device, "../shaders/triangle.vert.spv");
    VkShaderModule                         frag_shader        = load_shader(device, "../shaders/triangle.frag.spv");
    VkPipelineShaderStageCreateInfo        vert_shader_stage  = vk_lib::pipeline_shader_stage_create_info(VK_SHADER_STAGE_VERTEX_BIT, vert_shader);
    VkPipelineShaderStageCreateInfo        frag_shader_stage  = vk_lib::pipeline_shader_stage_create_info(VK_SHADER_STAGE_FRAGMENT_BIT, frag_shader);
    std::array                             shader_stages      = {vert_shader_stage, frag_shader_stage};
    VkPipelineVertexInputStateCreateInfo   vertex_input_state = vk_lib::pipeline_vertex_input_state_create_info();
    VkPipelineInputAssemblyStateCreateInfo input_assembly_state =
        vk_lib::pipeline_input_assembly_state_create_info(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    VkPipelineViewportStateCreateInfo      viewport_state = vk_lib::pipeline_viewport_state_create_info(nullptr, nullptr);
    VkPipelineRasterizationStateCreateInfo rasterization_state =
        vk_lib::pipeline_rasterization_state_create_info(VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_CLOCKWISE);
    VkPipelineMultisampleStateCreateInfo multisample_state            = vk_lib::pipeline_multisample_state_create_info(VK_SAMPLE_COUNT_4_BIT);
    VkPipelineColorBlendAttachmentState  color_blend_attachment_state = vk_lib::pipeline_color_blend_attachment_state();
    std::array                           color_blends                 = {color_blend_attachment_state};
    VkPipelineColorBlendStateCreateInfo  color_blend_state            = vk_lib::pipeline_color_blend_state_create_info(color_blends);
    std::array                           dynamic_state_types          = {VK_DYNAMIC_STATE_SCISSOR, VK_DYNAMIC_STATE_VIEWPORT};
    VkPipelineDynamicStateCreateInfo     dynamic_state                = vk_lib::pipeline_dynamic_state_create_info(dynamic_state_types);
    VkGraphicsPipelineCreateInfo         graphics_pipeline_ci         = vk_lib::graphics_pipeline_create_info(
        pipeline_layout, nullptr, shader_stages, &vertex_input_state, &input_assembly_state, &viewport_state, &rasterization_state,
        &multisample_state, &color_blend_state, nullptr, &dynamic_state, nullptr, 0, 0, nullptr, 0, &rendering_create_info);

    VkPipeline pipeline;
    vkCreateGraphicsPipelines(device, nullptr, 1, &graphics_pipeline_ci, nullptr, &pipeline);

    GraphicsPipeline graphics_pipeline{};
    graphics_pipeline.pipeline        = pipeline;
    graphics_pipeline.pipeline_layout = pipeline_layout;
    graphics_pipeline.vert_shader     = vert_shader;
    graphics_pipeline.frag_shader     = frag_shader;

    return graphics_pipeline;
}

static VmaAllocator allocator_create(const VkContext* vk_context) {
    // since I'm using volk, have to specify function pointers for vma
    VmaVulkanFunctions vma_vulkan_functions{};
    vma_vulkan_functions.vkAllocateMemory                        = vkAllocateMemory;
    vma_vulkan_functions.vkBindBufferMemory                      = vkBindBufferMemory;
    vma_vulkan_functions.vkBindImageMemory                       = vkBindImageMemory;
    vma_vulkan_functions.vkCreateBuffer                          = vkCreateBuffer;
    vma_vulkan_functions.vkCreateImage                           = vkCreateImage;
    vma_vulkan_functions.vkDestroyBuffer                         = vkDestroyBuffer;
    vma_vulkan_functions.vkDestroyImage                          = vkDestroyImage;
    vma_vulkan_functions.vkFlushMappedMemoryRanges               = vkFlushMappedMemoryRanges;
    vma_vulkan_functions.vkFreeMemory                            = vkFreeMemory;
    vma_vulkan_functions.vkGetBufferMemoryRequirements           = vkGetBufferMemoryRequirements;
    vma_vulkan_functions.vkGetImageMemoryRequirements            = vkGetImageMemoryRequirements;
    vma_vulkan_functions.vkGetPhysicalDeviceMemoryProperties     = vkGetPhysicalDeviceMemoryProperties;
    vma_vulkan_functions.vkGetPhysicalDeviceProperties           = vkGetPhysicalDeviceProperties;
    vma_vulkan_functions.vkInvalidateMappedMemoryRanges          = vkInvalidateMappedMemoryRanges;
    vma_vulkan_functions.vkMapMemory                             = vkMapMemory;
    vma_vulkan_functions.vkUnmapMemory                           = vkUnmapMemory;
    vma_vulkan_functions.vkCmdCopyBuffer                         = vkCmdCopyBuffer;
    vma_vulkan_functions.vkGetInstanceProcAddr                   = vkGetInstanceProcAddr;
    vma_vulkan_functions.vkGetDeviceProcAddr                     = vkGetDeviceProcAddr;
    vma_vulkan_functions.vkGetBufferMemoryRequirements2KHR       = vkGetBufferMemoryRequirements2KHR;
    vma_vulkan_functions.vkGetImageMemoryRequirements2KHR        = vkGetImageMemoryRequirements2KHR;
    vma_vulkan_functions.vkBindBufferMemory2KHR                  = vkBindBufferMemory2KHR;
    vma_vulkan_functions.vkBindImageMemory2KHR                   = vkBindImageMemory2KHR;
    vma_vulkan_functions.vkGetPhysicalDeviceMemoryProperties2KHR = vkGetPhysicalDeviceMemoryProperties2KHR;
    vma_vulkan_functions.vkGetDeviceBufferMemoryRequirements     = vkGetDeviceBufferMemoryRequirements;
    vma_vulkan_functions.vkGetDeviceImageMemoryRequirements      = vkGetDeviceImageMemoryRequirements;

    VmaAllocatorCreateInfo allocatorCreateInfo = {};
    allocatorCreateInfo.vulkanApiVersion       = VK_API_VERSION_1_3;
    allocatorCreateInfo.physicalDevice         = vk_context->physical_device;
    allocatorCreateInfo.device                 = vk_context->device;
    allocatorCreateInfo.instance               = vk_context->instance;
    allocatorCreateInfo.pVulkanFunctions       = &vma_vulkan_functions;

    VmaAllocator allocator;
    VK_CHECK(vmaCreateAllocator(&allocatorCreateInfo, &allocator));

    return allocator;
}

static void create_render_resources(Renderer* renderer) {
    VkContext*              vk_ctx        = &renderer->vk_context;
    const SwapchainContext* swapchain_ctx = &renderer->swapchain_context;

    VkExtent3D msaa_image_extent{};
    msaa_image_extent.width  = swapchain_ctx->extent.width;
    msaa_image_extent.height = swapchain_ctx->extent.height;
    msaa_image_extent.depth  = 1;

    VkImageCreateInfo msaa_image_ci =
        vk_lib::image_create_info(swapchain_ctx->surface_format.format, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                                  msaa_image_extent, VK_IMAGE_LAYOUT_UNDEFINED, 1, 1, VK_SAMPLE_COUNT_4_BIT);

    VmaAllocationCreateInfo allocation_ci{};
    allocation_ci.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;

    VK_CHECK(vmaCreateImage(renderer->allocator, &msaa_image_ci, &allocation_ci, &renderer->msaa_color_image.image,
                            &renderer->msaa_color_image.allocation, &renderer->msaa_color_image.allocation_info));

    VkImageSubresourceRange msaa_subresource_range = vk_lib::image_subresource_range(VK_IMAGE_ASPECT_COLOR_BIT);
    VkImageViewCreateInfo   msaa_image_view_ci =
        vk_lib::image_view_create_info(swapchain_ctx->surface_format.format, renderer->msaa_color_image.image, &msaa_subresource_range);
    vkCreateImageView(vk_ctx->device, &msaa_image_view_ci, nullptr, &renderer->msaa_color_image.image_view);
}

static void destroy_render_resources(Renderer* renderer) {
    vmaDestroyImage(renderer->allocator, renderer->msaa_color_image.image, renderer->msaa_color_image.allocation);
    vkDestroyImageView(renderer->vk_context.device, renderer->msaa_color_image.image_view, nullptr);
}

Renderer renderer_create() {
    if (!glfwInit()) {
        abort_message("GLFW cannot be initialized");
    }
    // glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    Renderer renderer{};

    renderer.window     = glfwCreateWindow(1920, 1080, "Photometric Camera", nullptr, nullptr);
    renderer.vk_context = vk_context_create(renderer.window);
    VkContext* vk_ctx   = &renderer.vk_context;

    renderer.swapchain_context            = swapchain_context_create(vk_ctx->physical_device, vk_ctx->device, vk_ctx->surface, renderer.window);
    const SwapchainContext* swapchain_ctx = &renderer.swapchain_context;

    const VkCommandPoolCreateInfo command_pool_ci =
        vk_lib::command_pool_create_info(vk_ctx->queue_family, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
    vkCreateCommandPool(vk_ctx->device, &command_pool_ci, nullptr, &vk_ctx->frame_command_pool);

    renderer.allocator = allocator_create(&renderer.vk_context);

    create_render_resources(&renderer);

    renderer.frames = frames_create(vk_ctx->device, vk_ctx->frame_command_pool, swapchain_ctx->image_views, swapchain_ctx->images,
                                    vk_ctx->queue_family, renderer.msaa_color_image.image_view);

    renderer.graphics_pipeline =
        create_graphics_pipeline(vk_ctx->device, swapchain_ctx->surface_format.format, swapchain_ctx->extent.width, swapchain_ctx->extent.height);
    return renderer;
}

void renderer_draw(Renderer* renderer) {

    VkContext*        vk_ctx        = &renderer->vk_context;
    SwapchainContext* swapchain_ctx = &renderer->swapchain_context;
    const uint32_t    frame_index   = renderer->curr_frame % swapchain_ctx->images.size();
    const Frame*      current_frame = &renderer->frames[frame_index];

    VkCommandBuffer command_buffer = current_frame->command_buffer;

    VK_CHECK(vkWaitForFences(vk_ctx->device, 1, &current_frame->in_flight_fence, true, UINT64_MAX));
    VK_CHECK(vkResetFences(vk_ctx->device, 1, &current_frame->in_flight_fence));

    uint32_t swapchain_image_index;
    VkResult swapchain_result = vkAcquireNextImageKHR(vk_ctx->device, swapchain_ctx->swapchain, UINT64_MAX, current_frame->image_available_semaphore,
                                                      nullptr, &swapchain_image_index);

    if (swapchain_result == VK_ERROR_OUT_OF_DATE_KHR || swapchain_result == VK_SUBOPTIMAL_KHR) {
        swapchain_context_recreate(swapchain_ctx, vk_ctx->physical_device, vk_ctx->device, vk_ctx->surface, renderer->window);
        destroy_render_resources(renderer);
        create_render_resources(renderer);
        return;
    }
    VK_CHECK(vkResetCommandBuffer(command_buffer, 0));

    VkCommandBufferBeginInfo begin_info = vk_lib::command_buffer_begin_info(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
    VK_CHECK(vkBeginCommandBuffer(command_buffer, &begin_info));

    const VkViewport viewport = vk_lib::viewport(static_cast<float>(swapchain_ctx->extent.width), static_cast<float>(swapchain_ctx->extent.height));
    const VkRect2D   scissor  = vk_lib::rect_2d(swapchain_ctx->extent);

    vkCmdSetViewport(command_buffer, 0, 1, &viewport);

    vkCmdSetScissor(command_buffer, 0, 1, &scissor);

    const VkImageSubresourceRange subresource_range = vk_lib::image_subresource_range(VK_IMAGE_ASPECT_COLOR_BIT);
    const VkImageMemoryBarrier2   msaa_draw_image_memory_barrier =
        vk_lib::image_memory_barrier_2(renderer->msaa_color_image.image, subresource_range, VK_IMAGE_LAYOUT_UNDEFINED,
                                       VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, vk_ctx->queue_family, vk_ctx->queue_family, VK_PIPELINE_STAGE_2_NONE,
                                       VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR, VK_ACCESS_2_NONE, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT);

    VkImage     swapchain_image      = swapchain_ctx->images[swapchain_image_index];
    VkImageView swapchain_image_view = swapchain_ctx->image_views[swapchain_image_index];

    const VkImageMemoryBarrier2 resolve_draw_image_memory_barrier =
        vk_lib::image_memory_barrier_2(swapchain_image, subresource_range, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                                       vk_ctx->queue_family, vk_ctx->queue_family, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR,
                                       VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR, VK_ACCESS_2_NONE, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT);

    std::array draw_image_memory_barriers = {msaa_draw_image_memory_barrier, resolve_draw_image_memory_barrier};

    const VkDependencyInfo draw_dependency_info = vk_lib::dependency_info_batch(draw_image_memory_barriers, {}, {});
    vkCmdPipelineBarrier2(command_buffer, &draw_dependency_info);

    VkClearValue clear_value{};
    clear_value.color                               = {0, 0, 0, 0};
    VkRenderingAttachmentInfo color_attachment_info = vk_lib::rendering_attachment_info(
        renderer->msaa_color_image.image_view, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_ATTACHMENT_LOAD_OP_CLEAR,
        VK_ATTACHMENT_STORE_OP_DONT_CARE, &clear_value, VK_RESOLVE_MODE_AVERAGE_BIT, swapchain_image_view, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

    std::array               color_attachment_infos = {color_attachment_info};
    const VkRect2D           render_area            = vk_lib::rect_2d(swapchain_ctx->extent);
    const VkRenderingInfoKHR rendering_info         = vk_lib::rendering_info(render_area, color_attachment_infos);

    vkCmdBeginRenderingKHR(command_buffer, &rendering_info);

    vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, renderer->graphics_pipeline.pipeline);

    vkCmdDraw(command_buffer, 3, 1, 0, 0);

    vkCmdEndRenderingKHR(command_buffer);

    const VkImageMemoryBarrier2 resolve_present_image_memory_barrier =
        vk_lib::image_memory_barrier_2(swapchain_image, subresource_range, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                                       vk_ctx->queue_family, vk_ctx->queue_family, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR,
                                       VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_2_NONE);

    const VkDependencyInfo present_dependency_info = vk_lib::dependency_info(&resolve_present_image_memory_barrier, nullptr, nullptr);
    vkCmdPipelineBarrier2(command_buffer, &present_dependency_info);

    VK_CHECK(vkEndCommandBuffer(command_buffer));

    VkSubmitInfo2 submit_info_2 = vk_lib::submit_info_2(&current_frame->command_buffer_submit_info, &current_frame->wait_semaphore_submit_info,
                                                        &current_frame->signal_semaphore_submit_info);

    VK_CHECK(vkQueueSubmit2(vk_ctx->graphics_queue, 1, &submit_info_2, current_frame->in_flight_fence));

    VkPresentInfoKHR present = vk_lib::present_info(&swapchain_ctx->swapchain, &swapchain_image_index, &current_frame->render_finished_semaphore);

    swapchain_result = vkQueuePresentKHR(vk_ctx->present_queue, &present);

    if (swapchain_result == VK_ERROR_OUT_OF_DATE_KHR || swapchain_result == VK_SUBOPTIMAL_KHR) {
        swapchain_context_recreate(swapchain_ctx, vk_ctx->physical_device, vk_ctx->device, vk_ctx->surface, renderer->window);
        destroy_render_resources(renderer);
        create_render_resources(renderer);
        return;
    }
    renderer->curr_frame++;
}
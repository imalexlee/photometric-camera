#include "renderer.h"

Renderer renderer_create() {
    if (!glfwInit()) {
        abort_message("GLFW cannot be initialized");
    }
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    Renderer renderer{};

    renderer.window     = glfwCreateWindow(640, 480, "Photometric Camera", nullptr, nullptr);
    renderer.vk_context = vk_context_create(renderer.window);
    VkContext* vk_ctx   = &renderer.vk_context;

    renderer.swapchain_context            = swapchain_context_create(vk_ctx->physical_device, vk_ctx->device, vk_ctx->surface, renderer.window);
    const SwapchainContext* swapchain_ctx = &renderer.swapchain_context;

    VkCommandPoolCreateInfo command_pool_ci = vk_lib::command_pool_create_info(vk_ctx->queue_family, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
    vkCreateCommandPool(vk_ctx->device, &command_pool_ci, nullptr, &vk_ctx->frame_command_pool);

    renderer.frames =
        frames_create(vk_ctx->device, vk_ctx->frame_command_pool, swapchain_ctx->image_views, swapchain_ctx->images, vk_ctx->queue_family);

    return renderer;
}
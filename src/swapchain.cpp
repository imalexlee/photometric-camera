#include "swapchain.h"

SwapchainContext swapchain_context_create(VkPhysicalDevice physical_device, VkDevice device, VkSurfaceKHR surface, GLFWwindow* window) {
    std::vector<VkSurfaceFormatKHR> surface_formats;

    uint32_t format_count = 0;
    VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &format_count, nullptr));

    surface_formats.resize(format_count);
    VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &format_count, surface_formats.data()));

    VkSurfaceFormatKHR format = surface_formats[0];
    for (const VkSurfaceFormatKHR& available_format : surface_formats) {
        if (available_format.format == VK_FORMAT_B8G8R8A8_SRGB && available_format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            format = available_format;
        }
    }
    VkSurfaceCapabilitiesKHR capabilities;
    VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, surface, &capabilities));

    VkExtent2D swapchain_extent{};
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        swapchain_extent = capabilities.currentExtent;
    } else {
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);
        swapchain_extent.width  = std::clamp(static_cast<uint32_t>(width), capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
        swapchain_extent.height = std::clamp(static_cast<uint32_t>(height), capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
    }
    // triple buffering
    uint32_t image_count = std::max(capabilities.minImageCount, 3u);
    if (capabilities.maxImageCount != 0) {
        image_count = std::min(image_count, capabilities.maxImageCount);
    }

    VkSwapchainCreateInfoKHR swapchain_ci =
        vk_lib::swapchain_create_info(surface, image_count, format.format, format.colorSpace, swapchain_extent, capabilities.currentTransform);
    VkSwapchainKHR swapchain;
    VK_CHECK(vkCreateSwapchainKHR(device, &swapchain_ci, nullptr, &swapchain));

    SwapchainContext swapchain_context{};
    swapchain_context.extent         = swapchain_extent;
    swapchain_context.surface_format = format;
    swapchain_context.swapchain      = swapchain;

    uint32_t swapchain_image_count = 0;
    VK_CHECK(vkGetSwapchainImagesKHR(device, swapchain, &swapchain_image_count, nullptr));
    swapchain_context.images.resize(swapchain_image_count);
    VK_CHECK(vkGetSwapchainImagesKHR(device, swapchain, &swapchain_image_count, swapchain_context.images.data()));

    swapchain_context.image_views.reserve(swapchain_context.images.size());
    for (VkImage image : swapchain_context.images) {
        VkImageSubresourceRange subresource_range = vk_lib::image_subresource_range(VK_IMAGE_ASPECT_COLOR_BIT);
        VkImageViewCreateInfo   image_view_ci     = vk_lib::image_view_create_info(format.format, image, &subresource_range);
        VkImageView             image_view;
        VK_CHECK(vkCreateImageView(device, &image_view_ci, nullptr, &image_view));
        swapchain_context.image_views.push_back(image_view);
    }

    return swapchain_context;
}

void swapchain_context_destroy(SwapchainContext* swapchain_context, VkDevice device) {
    swapchain_context->images.clear();
    for (VkImageView image_view : swapchain_context->image_views) {
        vkDestroyImageView(device, image_view, nullptr);
    }
    swapchain_context->image_views.clear();
    vkDestroySwapchainKHR(device, swapchain_context->swapchain, nullptr);
}

void swapchain_context_recreate(SwapchainContext* swapchain_context, VkPhysicalDevice physical_device, VkDevice device, VkSurfaceKHR surface,
                                GLFWwindow* window) {
    vkDeviceWaitIdle(device);
    swapchain_context_destroy(swapchain_context, device);
    *swapchain_context = swapchain_context_create(physical_device, device, surface, window);
}
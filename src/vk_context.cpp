#include "vk_context.h"

VkInstance create_instance() {
    if (!glfwVulkanSupported()) {
        abort_message("GLFW cannot find the vulkan loader and an ICD");
    }
    uint32_t                 glfw_extension_count = 0;
    const char**             glfw_extensions      = glfwGetRequiredInstanceExtensions(&glfw_extension_count);
    std::vector<const char*> extensions{};
    extensions.reserve(glfw_extension_count + 2);
    extensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
    for (uint32_t i = 0; i < glfw_extension_count; i++) {
        extensions.push_back(glfw_extensions[i]);
    }
    std::vector<const char*> layers;
#ifndef NDEBUG
    layers.push_back("VK_LAYER_KHRONOS_validation");
#endif
    const VkApplicationInfo    app_info    = vk_lib::application_info("Photometric Camera", "engine name", VK_API_VERSION_1_3);
    const VkInstanceCreateInfo instance_ci = vk_lib::instance_create_info(&app_info, layers, extensions);
    VkInstance                 instance;
    VK_CHECK(vk_lib::create_instance_with_entrypoints(&instance_ci, &instance));

    return instance;
}

static VkPhysicalDevice select_physical_device(VkInstance instance) {
    // Find a device that supports vulkan 1.3. Prefer discrete GPU's
    std::vector<VkPhysicalDevice> physical_devices;
    VK_CHECK(vk_lib::enumerate_physical_devices(instance, &physical_devices));
    VkPhysicalDevice chosen_device = nullptr;
    for (const VkPhysicalDevice& physical_device : physical_devices) {
        VkPhysicalDeviceProperties properties;
        vkGetPhysicalDeviceProperties(physical_device, &properties);
        if (properties.apiVersion < VK_API_VERSION_1_3) {
            continue;
        }
        chosen_device = physical_device;
        if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
            return chosen_device;
        }
    }
    if (chosen_device == nullptr) {
        abort_message("Could not find a suitable physical device");
    }
    return chosen_device;
}

static uint32_t select_queue_family(VkPhysicalDevice physical_device, VkSurfaceKHR surface) {
    const std::vector<VkQueueFamilyProperties> queue_family_properties = vk_lib::get_physical_device_queue_family_properties(physical_device);
    // Find a queue family with both graphics and presentation capabilities
    for (uint32_t i = 0; i < queue_family_properties.size(); i++) {
        const VkQueueFamilyProperties* family_properties = &queue_family_properties[i];
        if (family_properties->queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            VkBool32 present_supported = VK_FALSE;
            vkGetPhysicalDeviceSurfaceSupportKHR(physical_device, i, surface, &present_supported);
            if (present_supported) {
                return i;
            }
        }
    }
    abort_message("Could not find a queue family with both graphics and presentation supported.");
}

VkDevice create_logical_device(VkPhysicalDevice physical_device, uint32_t queue_family) {
    std::array              queue_priorities   = {1.f};
    VkDeviceQueueCreateInfo queue_ci           = vk_lib::device_queue_create_info(queue_family, 1, queue_priorities);
    std::array              queue_create_infos = {queue_ci};

    std::array device_extensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME, VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME, VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME,
                                    VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME};

    VkPhysicalDeviceVulkan13Features vk_1_3_features = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
    vk_1_3_features.dynamicRendering                 = VK_TRUE;
    vk_1_3_features.synchronization2                 = VK_TRUE;

    VkPhysicalDeviceVulkan12Features vk_1_2_features              = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES};
    vk_1_2_features.descriptorBindingVariableDescriptorCount      = VK_TRUE;
    vk_1_2_features.descriptorBindingUniformBufferUpdateAfterBind = VK_TRUE;
    vk_1_2_features.shaderSampledImageArrayNonUniformIndexing     = VK_TRUE;
    vk_1_2_features.runtimeDescriptorArray                        = VK_TRUE;
    vk_1_2_features.bufferDeviceAddress                           = VK_TRUE;
    vk_1_2_features.descriptorIndexing                            = VK_TRUE;
    vk_1_2_features.scalarBlockLayout                             = VK_TRUE;
    vk_1_2_features.pNext                                         = &vk_1_3_features;

    VkPhysicalDeviceFeatures2 physical_device_features_2  = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2_KHR};
    physical_device_features_2.features.samplerAnisotropy = VK_TRUE;
    physical_device_features_2.pNext                      = &vk_1_2_features;

    VkDeviceCreateInfo device_ci = vk_lib::device_create_info(queue_create_infos, device_extensions, nullptr, &physical_device_features_2);
    VkDevice           device;
    vk_lib::create_device_with_entrypoints(physical_device, &device_ci, &device);

    return device;
}

VkContext vk_context_create(GLFWwindow* window) {
    VkContext vk_context{};
    vk_context.instance        = create_instance();
    vk_context.physical_device = select_physical_device(vk_context.instance);
    VK_CHECK(glfwCreateWindowSurface(vk_context.instance, window, nullptr, &vk_context.surface));
    // only using one queue family for now. we need graphics and present on the same family
    vk_context.queue_family = select_queue_family(vk_context.physical_device, vk_context.surface);
    vk_context.device       = create_logical_device(vk_context.physical_device, vk_context.queue_family);
    vkGetDeviceQueue(vk_context.device, vk_context.queue_family, 0, &vk_context.graphics_queue);
    vkGetDeviceQueue(vk_context.device, vk_context.queue_family, 0, &vk_context.present_queue);
    return vk_context;
}
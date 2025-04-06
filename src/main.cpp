
#include <vk_lib/commands.h>
#include <vk_lib/core.h>
#include <vulkan/vk_enum_string_helper.h>
#define VK_CHECK(x)                                                                                                                                  \
    do {                                                                                                                                             \
        VkResult err = x;                                                                                                                            \
        if (err) {                                                                                                                                   \
            std::cerr << "Detected Vulkan error: " << string_VkResult(err) << std::endl;                                                             \
            abort();                                                                                                                                 \
        }                                                                                                                                            \
    } while (0)

int main() {
    const VkApplicationInfo app_info   = vk_lib::application_info("Photometric Camera", "My Engine", VK_API_VERSION_1_3,
                                                                  VK_MAKE_API_VERSION(0, 1, 0, 0), VK_MAKE_API_VERSION(0, 1, 0, 0));
    std::array              extensions = {"VK_KHR_surface"};

    std::vector<const char*> layers;
#ifndef NDEBUG
    layers.push_back("VK_LAYER_KHRONOS_validation");
#endif
    const VkInstanceCreateInfo instance_info = vk_lib::instance_create_info(&app_info, layers, extensions);

    VkInstance instance;
    VK_CHECK(vk_lib::create_instance_with_entrypoints(&instance_info, &instance));
}
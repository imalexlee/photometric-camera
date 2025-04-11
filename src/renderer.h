#pragma once
#include "common.h"

#include <frame.h>
#include <swapchain.h>
#include <vk_context.h>

struct GraphicsPipeline {
    VkPipeline       pipeline{};
    VkPipelineLayout pipeline_layout{};
    VkShaderModule   vert_shader{};
    VkShaderModule   frag_shader{};
};

struct Renderer {
    VkContext          vk_context{};
    SwapchainContext   swapchain_context{};
    std::vector<Frame> frames{};
    uint64_t           curr_frame{};
    GraphicsPipeline   graphics_pipeline{};
    GLFWwindow*        window{};
    VmaAllocator       allocator{};
    AllocatedImage     msaa_color_image{};
};

Renderer renderer_create();

void renderer_draw(Renderer* renderer);

#pragma once
#include "common.h"

#include "loader.h"
#include <frame.h>
#include <swapchain.h>
#include <vk_context.h>

struct GraphicsPipeline
{
    VkPipeline pipeline{};
    VkPipelineLayout pipeline_layout{};
    VkShaderModule vert_shader{};
    VkShaderModule frag_shader{};
};

struct SceneData
{
    glm::mat4 view{};
    glm::mat4 proj{};
    glm::vec4 eye_pos{};
};

struct Material
{
    TextureInfo base_color_texture;
    TextureInfo metallic_roughness_texture;
    TextureInfo normal_texture;
    glm::vec4 base_color_factors;
    float metallic_factor;
    float roughness_factor;
};

struct Texture
{
    AllocatedImage image;
    VkSampler sampler{nullptr};
};

struct Renderer
{
    VkContext vk_context{};
    SwapchainContext swapchain_context{};
    std::vector<Frame> frames{};
    uint64_t curr_frame{};
    GraphicsPipeline graphics_pipeline{};
    GLFWwindow* window{};
    VmaAllocator allocator{};
    AllocatedImage msaa_color_image{};
    // AllocatedImage        depth_image{};
    VkDescriptorPool descriptor_pool{};
    VkDescriptorSet descriptor_set{};
    VkDescriptorSetLayout descriptor_set_layout{};

    SceneData scene_data{};
    AllocatedBuffer material_buffer{};

    AllocatedImage default_texture_image;
    VkSampler default_sampler{};

    uint32_t material_count{};
    uint32_t texture_count{};
};

Renderer renderer_create();

void renderer_draw(Renderer* renderer);

#pragma once
#include "common.h"

#include "loader.h"
#include "window.h"
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
    glm::vec3 eye_pos{};
};

struct PushConstants
{
    glm::mat4 model_transform{};
    VkDeviceAddress vertex_buf_address{};
    uint32_t material_index{};
};

struct Material
{
    TextureInfo base_color_texture{};
    TextureInfo metallic_roughness_texture{};
    TextureInfo normal_texture{};
    TextureInfo occlusion_texture{};
    TextureInfo emissive_texture{};

    glm::vec4 base_color_factors;
    glm::vec3 emissive_factors;
    float metallic_factor;
    float roughness_factor;
    float occlusion_strength;
    float normal_scale;
};

struct Texture
{
    AllocatedImage image;
    VkSampler sampler{nullptr};
};

// loosely matches a GltfPrimitive
struct DrawObject
{
    glm::mat4 transform{};
    AllocatedBuffer index_buffer{};
    AllocatedBuffer vertex_buffer{};
    VkIndexType index_type{};
    uint32_t index_count{};
    uint32_t material_index{};
};

struct Renderer
{
    VkContext vk_context{};
    SwapchainContext swapchain_context{};

    std::vector<Frame> frames{};
    Window window{};
    uint64_t curr_frame{};
    GraphicsPipeline opaque_graphics_pipeline{};
    GraphicsPipeline transparent_graphics_pipeline{};
    VmaAllocator allocator{};
    AllocatedImage msaa_color_image{};
    AllocatedImage depth_image{};
    VkDescriptorPool descriptor_pool{};
    std::vector<VkDescriptorSet> scene_descriptor_sets{};
    VkDescriptorSet asset_descriptor_set{};
    VkDescriptorSetLayout scene_descriptor_set_layout{};
    VkDescriptorSetLayout asset_descriptor_set_layout{};

    SceneData scene_data{};

    std::vector<GltfAsset> assets{};
    AllocatedBuffer material_buffer{};
    std::vector<AllocatedBuffer> scene_data_buffers{};
    AllocatedImage default_texture_image;
    VkSampler default_sampler{};
    uint32_t material_count{};
    uint32_t texture_count{};
    std::vector<DrawObject> opaque_draws;
    std::vector<DrawObject> transparent_draws;

    float frame_time{};
};

Renderer renderer_create();

void renderer_draw(Renderer* renderer);

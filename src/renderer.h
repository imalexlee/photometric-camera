#pragma once
#include "common.h"

#include "window.h"
#include <frame.h>
#include <swapchain.h>
#include <vk_context.h>
#include <vk_gltf/loader.h>

struct GraphicsPipeline {
    VkPipeline       pipeline{};
    VkPipelineLayout pipeline_layout{};
    VkShaderModule   vert_shader{};
    VkShaderModule   frag_shader{};
};

struct SceneData {
    glm::mat4 view{};
    glm::mat4 proj{};
    glm::mat4 light_transform{};
    glm::vec3 eye_pos{};
    glm::vec3 sun_dir{};
};

struct PushConstants {
    glm::mat4       model_transform{};
    VkDeviceAddress vertex_buf_address{};
    uint32_t        material_index{};
};

struct Material {
    vk_gltf::TextureInfo base_color_texture{};
    vk_gltf::TextureInfo metallic_roughness_texture{};
    vk_gltf::TextureInfo normal_texture{};
    vk_gltf::TextureInfo occlusion_texture{};
    vk_gltf::TextureInfo emissive_texture{};

    // extension textures
    vk_gltf::TextureInfo clearcoat_texture{};
    vk_gltf::TextureInfo clearcoat_roughness_texture{};
    vk_gltf::TextureInfo clearcoat_normal_texture{};

    glm::vec4 base_color_factors{};
    glm::vec3 emissive_factors{};
    float     metallic_factor{};
    float     roughness_factor{};
    float     occlusion_strength{};
    float     normal_scale{};

    // extension factors
    float clearcoat_factor{};
    float clearcoat_roughness_factor{};
};

struct Texture {
    AllocatedImage image;
    VkSampler      sampler{nullptr};
};

struct Bounds {
    glm::vec3 origin{};
    glm::vec3 extent{};
};

// loosely matches a GltfPrimitive
struct DrawObject {
    glm::mat4           transform{};
    AllocatedBuffer     index_buffer{};
    VkIndexType         index_type{};
    uint32_t            index_count{};
    AllocatedBuffer     vertex_buffer{};
    Bounds              bounds{};
    VkFrontFace         front_face{};
    VkPrimitiveTopology topology{};
    bool                double_sided{};
    uint32_t            material_index{};
};

struct Renderer {
    VkContext        vk_context{};
    SwapchainContext swapchain_context{};

    std::vector<Frame>           frames{};
    Window                       window{};
    uint64_t                     curr_frame{};
    GraphicsPipeline             opaque_graphics_pipeline{};
    GraphicsPipeline             transparent_graphics_pipeline{};
    GraphicsPipeline             shadow_map_graphics_pipeline{};
    GraphicsPipeline             depth_pre_graphics_pipeline{};
    VmaAllocator                 allocator{};
    AllocatedImage               msaa_color_image{};
    AllocatedImage               depth_image{};
    AllocatedImage               shadow_map_image{};
    VkExtent3D                   shadow_map_extent{};
    VkDescriptorPool             descriptor_pool{};
    std::vector<VkDescriptorSet> shadow_descriptor_sets{};
    std::vector<VkDescriptorSet> scene_descriptor_sets{};
    VkDescriptorSet              asset_descriptor_set{};
    VkDescriptorSetLayout        shadow_descriptor_set_layout{};
    VkDescriptorSetLayout        scene_descriptor_set_layout{};
    VkDescriptorSetLayout        asset_descriptor_set_layout{};

    SceneData scene_data{};

    std::vector<vk_gltf::GltfAsset> assets{};
    AllocatedBuffer                 material_buffer{};
    std::vector<AllocatedBuffer>    main_scene_data_buffers{};
    std::vector<AllocatedBuffer>    shadow_scene_data_buffers{};
    AllocatedImage                  default_texture_image;
    VkSampler                       default_sampler{};
    uint32_t                        material_count{};
    uint32_t                        texture_count{};
    std::vector<DrawObject>         opaque_draws;
    std::vector<DrawObject>         transparent_draws;

    // for frustum culling
    std::vector<DrawObject> visible_opaque_draws;
    std::vector<DrawObject> visible_transparent_draws;

    float frame_time{};

    glm::mat4 light_transform{};
    glm::vec3 sun_dir{};
};

void renderer_create(Renderer* renderer);

void renderer_key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);

void renderer_recompile_pipelines(Renderer* renderer);

void renderer_draw(Renderer* renderer);

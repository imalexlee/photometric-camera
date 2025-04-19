#pragma once

#include "common.h"

struct Vertex {
    float color[4]{1, 0, 1, 1};
    float position[3]{};
    float normal[3]{};
    float tex_coord[2][2]{};
};

struct GltfTexture {
    std::optional<uint32_t> image;
    std::optional<uint32_t> sampler;
};

struct TextureInfo {
    uint32_t index{};
    uint32_t tex_coord{};
};

enum class GltfAlphaMode {
    opaque,
    mask,
    blend,
};

struct GltfMaterial {
    std::optional<TextureInfo> base_color_texture{};
    std::optional<TextureInfo> metallic_roughness_texture{};
    std::optional<TextureInfo> normal_texture{};

    float base_color_factors[4]{1, 1, 1, 1};
    float metallic_factor{1};
    float roughness_factor{1};

    GltfAlphaMode alpha_mode{};
};

enum class PrimitiveMode {
    points,
    lines,
    line_loop,
    line_strip,
    triangles,
    triangle_strip,
    triangle_fan,
};

struct GltfPrimitive {
    std::optional<AllocatedBuffer> index_buffer;
    VkIndexType                    index_type{VK_INDEX_TYPE_UINT16};
    uint32_t                       index_count{};
    AllocatedBuffer                vertex_buffer{};
    std::optional<uint32_t>        material;
    PrimitiveMode                  mode{PrimitiveMode::triangles};
};

struct GltfMesh {
    // todo: add morph target weights
    std::vector<GltfPrimitive> primitives{};
};

struct GltfNode {
    float                   local_transform[16]{};
    float                   world_transform[16]{};
    std::optional<uint32_t> mesh;
    std::vector<uint32_t>   children{};
};

struct GltfScene {
    std::vector<uint32_t> nodes;
};

struct GltfAsset {
    std::vector<GltfScene>      scenes{};
    std::vector<GltfNode>       nodes{};
    std::vector<GltfMesh>       meshes{};
    std::vector<GltfMaterial>   materials{};
    std::vector<GltfTexture>    textures{};
    std::vector<AllocatedImage> images{};
    std::vector<VkSampler>      samplers{};
};

struct LoadOptions {
    std::filesystem::path gltf_path{};
    std::filesystem::path cache_dir{};
};

[[nodiscard]] GltfAsset load_gltf(const LoadOptions* load_options, VkDevice device, VmaAllocator allocator, VkCommandPool command_pool, VkQueue queue,
                                  uint32_t queue_family_index);

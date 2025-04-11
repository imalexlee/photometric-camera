#pragma once

#include "common.h"

struct Vertex {
    float position[4]{};
    float normal[4]{};
    float tangent[4]{};
    // GLTF spec states that clients should support at least
    // 2 texture coordinates
    float tex_coord[2][2]{};
};

// holds allocated, compressed images from the loaded gltf
// struct GltfImages {
//     VkImage image{};
//     VkImage image_view{};
// };

struct TextureInfo {
    uint32_t index{};
    uint32_t tex_coord{};
};

struct GltfMaterial {};

struct GltfPrimitive {
    VkBuffer index_buffer{};
    VkBuffer vertices{};
    uint32_t material{};
};

struct GltfMesh {
    // todo: add morph target weights
    std::vector<GltfPrimitive> primitives{};
};

struct GltfNode {
    float                 local_transform[16]{};
    float                 world_transform[16]{};
    GltfMesh              mesh;
    std::vector<uint32_t> children{};
};

struct LoadOptions {
    std::filesystem::path gltf_path{};
    std::filesystem::path cache_dir{};
};

void load_gltf(const LoadOptions* load_options, VkDevice device, VmaAllocator allocator, VkCommandPool command_pool, VkQueue queue,
               uint32_t queue_family_index);

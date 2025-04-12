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

struct TextureInfo {
    uint32_t index{};
    uint32_t tex_coord{};
};

struct GltfMaterial {};

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
    VkBuffer      index_buffer{};
    VkBuffer      vertices{};
    int32_t       material{};
    PrimitiveMode mode{PrimitiveMode::triangles};
};

struct GltfMesh {
    // todo: add morph target weights
    std::vector<GltfPrimitive> primitives{};
};

struct GltfNode {
    float                 local_transform[16]{};
    float                 world_transform[16]{};
    uint32_t              mesh;
    std::vector<uint32_t> children{};
};

struct LoadOptions {
    std::filesystem::path gltf_path{};
    std::filesystem::path cache_dir{};
};

void load_gltf(const LoadOptions* load_options, VkDevice device, VmaAllocator allocator, VkCommandPool command_pool, VkQueue queue,
               uint32_t queue_family_index);

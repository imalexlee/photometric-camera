#include "loader.h"

#include <stb_image.h>

#define KHRONOS_STATIC
#include <ktx.h>

static std::string cgltf_result_to_string(cgltf_result result) {
    switch (result) {
    case cgltf_result_success:
        return "Success";
    case cgltf_result_data_too_short:
        return "Data too short";
    case cgltf_result_unknown_format:
        return "Unknown format";
    case cgltf_result_invalid_json:
        return "Invalid JSON";
    case cgltf_result_invalid_gltf:
        return "Invalid glTF";
    case cgltf_result_invalid_options:
        return "Invalid options";
    case cgltf_result_file_not_found:
        return "File not found";
    case cgltf_result_io_error:
        return "I/O error";
    case cgltf_result_out_of_memory:
        return "Out of memory";
    case cgltf_result_legacy_gltf:
        return "Legacy glTF";
    case cgltf_result_max_enum:
        return "Invalid enum value";
    default:
        return "Unknown result code";
    }
}

struct Vertex {
    float position[4]{};
    float normal[4]{};
    float tangent[4]{};
    // GLTF spec states that clients should support at least
    // 2 texture coordinates
    float tex_coord[2][2]{};
};

// holds allocated, compressed images from the loaded gltf
struct GltfImages {
    VkImage image{};
    VkImage image_view{};
};

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

// load gltf images, compress them, then create vulkan images and images views from them
void load_gltf_images(const cgltf_data* cgltf_data) {
    for (uint32_t i = 0; i < cgltf_data->images_count; i++) {
        int                      width, height, original_component_count;
        const cgltf_buffer_view* buffer_view = cgltf_data->images[i].buffer_view;

        unsigned char* img_data = stbi_load_from_memory(static_cast<uint8_t*>(buffer_view->buffer->data) + buffer_view->offset, buffer_view->size,
                                                        &width, &height, &original_component_count, 4);

        if (img_data == nullptr) {
            abort_message(stbi_failure_reason());
        }

        stbi_image_free(img_data);
    }
}

void load_gltf(const char* gltf_path) {
    cgltf_options options{};
    cgltf_data*   gltf_data = nullptr;

    cgltf_result result = cgltf_parse_file(&options, gltf_path, &gltf_data);
    if (result != cgltf_result_success) {
        const std::string message = "Cannot parse gltf/glb file with error: " + cgltf_result_to_string(result);
        abort_message(message);
    }

    result = cgltf_load_buffers(&options, gltf_data, gltf_path);
    if (result != cgltf_result_success) {
        const std::string message = "Cannot parse gltf/glb file with error: " + cgltf_result_to_string(result);
        abort_message(message);
    }

    result = cgltf_validate(gltf_data);
    if (result != cgltf_result_success) {
        const std::string message = "Cannot parse gltf/glb file with error: " + cgltf_result_to_string(result);
        abort_message(message);
    }

    load_gltf_images(gltf_data);

    cgltf_free(gltf_data);
}
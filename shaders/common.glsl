#extension GL_EXT_buffer_reference: enable
#extension GL_EXT_scalar_block_layout: enable
#extension GL_EXT_nonuniform_qualifier: enable

struct Vertex {
    vec4 color;
    vec3 position;
    vec3 normal;
    vec2 tex_coords[2];
};

layout (scalar, buffer_reference) readonly buffer VertexBuffer {
    Vertex vertices[];
};

struct TextureInfo {
    uint index;
    uint tex_coord;
};

struct Material {
    TextureInfo base_color_texture;
    TextureInfo metallic_roughness_texture;
    TextureInfo normal_texture;
    vec4 base_color_factors;
    float metallic_factor;
    float roughness_factor;
};

layout (set = 0, binding = 0) uniform SceneData {
    mat4 view;
    mat4 proj;
    vec4 eye_pos;
} scene_data;

layout (scalar, set = 0, binding = 1) readonly buffer MaterialBuffer {
    Material materials[];
} material_buf;

layout (set = 0, binding = 2) uniform sampler2D tex_samplers[];

layout (push_constant) uniform PushConstants {
    mat4 model_transform;
    VertexBuffer vertex_buffer;
    uint material_index;
} constants;
#version 450
#include "common.glsl"

layout (location = 0) out vec4 vert_position;
layout (location = 1) out vec4 vert_color;
layout (location = 2) out vec3 vert_normal;
layout (location = 3) out vec2 normal_uv;
layout (location = 4) out vec2 color_uv;
layout (location = 5) out vec2 occlusion_uv;
layout (location = 6) out vec2 metal_rough_uv;
layout (location = 7) out vec2 emissive_uv;
layout (location = 8) out vec4 vert_light_pos;

const mat4 bias_mat = mat4(
0.5, 0.0, 0.0, 0.0,
0.0, 0.5, 0.0, 0.0,
0.0, 0.0, 1.0, 0.0,
0.5, 0.5, 0.0, 1.0);

void main() {
    Vertex v = constants.vertex_buffer.vertices[gl_VertexIndex];

    vert_position = constants.model_transform * vec4(v.position.xyz, 1.f);
    vert_light_pos = bias_mat * scene_data.light_transform * vert_position;

    vert_color = v.color;
    vert_normal = mat3(constants.model_transform) * v.normal.xyz;

    gl_Position = scene_data.proj * scene_data.view * vert_position;

    Material mat = material_buf.materials[nonuniformEXT(constants.material_index)];

    normal_uv = v.tex_coords[mat.normal_texture.tex_coord];
    color_uv = v.tex_coords[mat.base_color_texture.tex_coord];
    occlusion_uv = v.tex_coords[mat.occlusion_texture.tex_coord];
    metal_rough_uv = v.tex_coords[mat.metallic_roughness_texture.tex_coord];
    emissive_uv = v.tex_coords[mat.emissive_texture.tex_coord];
}
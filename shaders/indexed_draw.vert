#version 450
#include "common.glsl"

layout (location = 0) out vec4 position;
layout (location = 1) out vec3 normal;
layout (location = 2) out vec2 color_uv;

void main() {
    Vertex v = constants.vertex_buffer.vertices[gl_VertexIndex];

    normal = mat3(constants.model_transform) * v.normal.xyz;
    position = constants.model_transform * vec4(v.position.xyz, 1.f);

    gl_Position = scene_data.proj * scene_data.view * position;

    Material mat = material_buf.materials[nonuniformEXT(constants.material_index)];
    color_uv = v.tex_coords[mat.base_color_texture.tex_coord];
}
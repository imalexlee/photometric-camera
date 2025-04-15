#version 450
#include "common.glsl"

layout (location = 0) in vec4 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 color_uv;

layout (location = 0) out vec4 out_color;

void main() {
    Material mat = material_buf.materials[nonuniformEXT(constants.material_index)];
    out_color = mat.base_color_factors * texture(tex_samplers[nonuniformEXT(mat.base_color_texture.index)], color_uv);
}
#version 450
#include "common.glsl"

layout (location = 0) in vec4 vert_position;
layout (location = 1) in vec4 vert_color;
layout (location = 2) in vec3 vert_normal;
layout (location = 3) in vec2 normal_uv;
layout (location = 4) in vec2 color_uv;
layout (location = 5) in vec2 occlusion_uv;
layout (location = 6) in vec2 metal_rough_uv;
layout (location = 7) in vec2 emissive_uv;

layout (location = 0) out vec4 out_color;

void main() {
    Material mat = material_buf.materials[nonuniformEXT(constants.material_index)];

    vec3 tex_normal = (texture(tex_samplers[nonuniformEXT(mat.normal_texture.index)], normal_uv).xyz * vec3(2.f) - vec3(1.f)) * vec3(mat.normal_scale);
    vec4 tex_color = texture(tex_samplers[nonuniformEXT(mat.base_color_texture.index)], color_uv).rgba;

    float occlusion = 1.f + mat.occlusion_strength * (texture(tex_samplers[nonuniformEXT(mat.occlusion_texture.index)], occlusion_uv).r - 1.f);

    vec2 metallic_roughness = texture(tex_samplers[nonuniformEXT(mat.metallic_roughness_texture.index)], metal_rough_uv).bg;
    float metallic = metallic_roughness.x * mat.metallic_factor;
    float roughness = metallic_roughness.y * mat.roughness_factor;

    vec3 light_dir = normalize(vec3(0, 1, 1));
    float n_dot_l = max(dot(light_dir, vert_normal), 0.1);

    tex_color *= n_dot_l;

    vec3 emissive = texture(tex_samplers[nonuniformEXT(mat.emissive_texture.index)], emissive_uv).rgb * mat.emissive_factors;

    out_color = mat.base_color_factors * tex_color;
    //        out_color = vec4(occlusion, occlusion, occlusion, 1);

    //    float el = n_dot_l;
    //    out_color = vec4(el, el, el, 1);
    //    out_color = vec4()
}
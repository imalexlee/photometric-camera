#version 450
#extension GL_ARB_shading_language_include: enable
#include "common.glsl"

layout (location = 0) in vec4 vert_position;
layout (location = 1) in vec4 vert_color;
layout (location = 2) in vec4 vert_tangent;
layout (location = 3) in vec3 vert_normal;
layout (location = 4) in vec4 vert_light_pos;
layout (location = 5) in vec2 normal_uv;
layout (location = 6) in vec2 color_uv;
layout (location = 7) in vec2 occlusion_uv;
layout (location = 8) in vec2 metal_rough_uv;
layout (location = 9) in vec2 emissive_uv;
layout (location = 10) in vec2 clearcoat_uv;
layout (location = 11) in vec2 clearcoat_rough_uv;
layout (location = 12) in vec2 clearcoat_normal_uv;

layout (location = 0) out vec4 out_color;

const float PI = 3.14159265359;
const float epsilon = 0.00001;

float distribution_ggx(vec3 normal, vec3 halfway, float roughness) {
    float roughness_2 = roughness * roughness;
    float n_dot_h = max(dot(normal, halfway), epsilon);
    float heaviside = step(epsilon, n_dot_h);
    float numerator = roughness_2 * heaviside;
    float denominator = n_dot_h * n_dot_h * (roughness_2 - 1) + 1;
    denominator = PI * denominator * denominator;
    return numerator / denominator;
}

float masking_g1_smith(vec3 normal, vec3 halfway, vec3 visibility_dir, float roughness) {
    float roughness_2 = roughness * roughness;
    float heaviside = step(epsilon, max(dot(halfway, visibility_dir), epsilon));
    float n_dot_v = dot(normal, visibility_dir);
    float n_dot_v_2 = max(n_dot_v * n_dot_v, epsilon);
    float n_dot_v_abs = max(abs(n_dot_v), epsilon);
    float numerator = heaviside;
    float denominator = n_dot_v_abs + sqrt(roughness_2 + (1 - roughness_2) * n_dot_v_2);
    return numerator / denominator;
}

float masking_g2_smith(vec3 normal, vec3 halfway, vec3 light_dir, vec3 view_dir, float roughness) {
    return masking_g1_smith(normal, halfway, light_dir, roughness) * masking_g1_smith(normal, halfway, view_dir, roughness);
}

float specular_brdf(vec3 normal, vec3 halfway, vec3 light_dir, vec3 view_dir, float roughness) {
    return masking_g2_smith(normal, halfway, light_dir, view_dir, roughness) * distribution_ggx(normal, halfway, roughness);
}

vec3 diffuse_brdf(vec3 color) {
    return (1.f / PI) * color;
}

vec3 conductor_fresnel(vec3 bsdf, vec3 f_0, vec3 view_dir, vec3 halfway) {
    float v_dot_h = dot(view_dir, halfway);
    float v_dot_h_abs = max(abs(v_dot_h), epsilon);
    return bsdf * (f_0 + (1 - f_0) * pow(1 - v_dot_h_abs, 5));
}

vec3 fresnel_mix(vec3 base, vec3 layer, vec3 view_dir, vec3 halfway) {
    float v_dot_h = dot(view_dir, halfway);
    float v_dot_h_abs = max(abs(v_dot_h), epsilon);
    float f_0 = 0.04;
    float fr = f_0 + (1 - f_0) * pow(1 - v_dot_h_abs, 5);
    return mix(base, layer, fr);
}

vec3 fresnel_coat(vec3 base, vec3 layer, float weight, vec3 view_dir, vec3 halfway) {
    float v_dot_h = dot(view_dir, halfway);
    float v_dot_h_abs = max(abs(v_dot_h), epsilon);
    float f_0 = 0.04;
    float fr = f_0 + (1 - f_0) * pow(1 - v_dot_h_abs, 5);
    return mix(base, layer, weight * fr);
}

vec3 ACESFilm(vec3 x)
{
    float a = 2.51f;
    float b = 0.03f;
    float c = 2.43f;
    float d = 0.59f;
    float e = 0.14f;
    return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.f, 1.f);
}

float compute_EV100(float aperture, float shutterTime, float ISO) {
    // EV number is defined as :
    // 2^ EV_s = N ^2 / t and EV_s = EV_100 + log2 ( S /100)
    // This gives
    // EV_s = log2 ( N ^2 / t )
    // EV_100 + log2 ( S /100) = log2 ( N ^2 / t )
    // EV_100 = log2 ( N ^2 / t ) - log2 ( S /100)
    // EV_100 = log2 ( N ^2 / t . 100 / S )
    return log2(sqrt(aperture) / shutterTime * 100 / ISO);
}

float convert_EV100_to_exposure(float EV100) {
    // Compute the maximum luminance possible with H_sbs sensitivity
    // maxLum = 78 / ( S * q ) * N ^2 / t
    // = 78 / ( S * q ) * 2^ EV_100
    // = 78 / (100 * 0.65) * 2^ EV_100
    // = 1.2 * 2^ EV
    // Reference : http :// en . wikipedia . org / wiki / Film_speed
    float max_luminance = 1.2f * pow(2.f, EV100);
    return 1.f / max_luminance;
}
void main() {
    Material mat = material_buf.materials[nonuniformEXT (constants.material_index)];

    vec3 tex_normal = texture(tex_samplers[nonuniformEXT (mat.normal_texture.index)], normal_uv).xyz;
    vec4 tex_color = texture(tex_samplers[nonuniformEXT (mat.base_color_texture.index)], color_uv).rgba;

    vec3 normal = vert_normal;


    // until i can find a branchless option, simply don't apply normal mapping if the tex_normal == vec(1)
    // since this means we read the default texture. aka, there is no normal map.
    mat3 TBN;
    if (tex_normal != vec3(1)){
        tex_normal = tex_normal* 2.f - 1.f;
        tex_normal *= vec3(mat.normal_scale, mat.normal_scale, 1);
        tex_normal = normalize(tex_normal);
        vec3 bitangent = cross(vert_normal, vec3(vert_tangent)) * -vert_tangent.w;
        TBN = mat3(vec3(vert_tangent), bitangent, vert_normal);
        normal = normalize(TBN * tex_normal);
    }


    float occlusion = 1.f + mat.occlusion_strength * (texture(tex_samplers[nonuniformEXT (mat.occlusion_texture.index)], occlusion_uv).r - 1.f);
    vec3 emissive = texture(tex_samplers[nonuniformEXT (mat.emissive_texture.index)], emissive_uv).rgb * mat.emissive_factors;


    vec2 metallic_roughness = texture(tex_samplers[nonuniformEXT (mat.metallic_roughness_texture.index)], metal_rough_uv).bg;
    float metallic = metallic_roughness.x * mat.metallic_factor;
    float roughness = metallic_roughness.y * mat.roughness_factor;

    vec3 view_dir = normalize(scene_data.eye_pos - vec3(vert_position));
    vec3 light_dir = scene_data.sun_dir;

    light_dir       = normalize(vec3(2, 4.5, 1));

    vec3 halfway_dir = normalize(light_dir + view_dir);

    vec4 albedo = mat.base_color_factors * tex_color;

    vec3 specular_brdf_val = vec3(specular_brdf(normal, halfway_dir, light_dir, view_dir, roughness));
    vec3 diffuse_brdf = diffuse_brdf(vec3(albedo));

    vec3 metal_brdf = conductor_fresnel(specular_brdf_val, albedo.rgb, view_dir, halfway_dir);
    vec3 dielectric_brdf = fresnel_mix(diffuse_brdf, specular_brdf_val, view_dir, halfway_dir);

    //    out_color = vec4(vec3(metallic), 1);
    //    return;
    vec3 material = mix(dielectric_brdf, metal_brdf, metallic);

    float n_dot_l = max(dot(normal, light_dir), 0.0);


    float clearcoat = texture(tex_samplers[nonuniformEXT (mat.clearcoat_texture.index)], clearcoat_uv).r * mat.clearcoat_factor;
    if (clearcoat != 0){

        float clearcoat_roughness = texture(tex_samplers[nonuniformEXT (mat.clearcoat_roughness_texture.index)], clearcoat_rough_uv).g * mat.clearcoat_roughness_factor;
        vec3 clearcoat_normal  = normal;
        vec3 tex_clearcoat_normal = texture(tex_samplers[nonuniformEXT (mat.clearcoat_normal_texture.index)], clearcoat_normal_uv).xyz;
        // only apply bump mapping if clearcoat normal isn't the default texture
        if (tex_clearcoat_normal != vec3(1)){
            tex_clearcoat_normal = tex_clearcoat_normal * 2.f - 1.f;
            tex_clearcoat_normal = normalize(TBN * tex_clearcoat_normal);
        }

        float clearcoat_brdf = specular_brdf(clearcoat_normal, halfway_dir, light_dir, view_dir, roughness);
        material = fresnel_coat(material, vec3(clearcoat_brdf), clearcoat, view_dir, halfway_dir);

    }

    // MANUAL EXPOSURE
    float aperature = 8;
    float shutter_time = 1 / 125.f;
    float iso = 100;

    float EV100 = compute_EV100(aperature, shutter_time, iso);
    float exposure = convert_EV100_to_exposure(EV100);

    // sun
    vec3 sun_color = vec3(0.99, 0.98, 0.95);
    vec3 illuminance = sun_color * 5326;// sun lux


    vec3 direct_luminance = material * illuminance * exposure * n_dot_l;

    float ambient_ratio = 0.05;
    vec3 ambient_illuminance = illuminance * ambient_ratio;// typically much lower than direct illuminance

    //    light_dir_factor = 1;
    vec3 ambient_contribution = ambient_illuminance * exposure * albedo.rgb * occlusion;
    float up_factor = max((dot(normal, vec3(0, 1, 0)) + 1.f) * 0.5f, 0.2);
    ambient_contribution *= up_factor;

    // PCF shadows
    int radius = 3;
    float shaow_texel_count = pow(radius * 2 + 1, 2);
    float shadow = shaow_texel_count;
    float texelSize = 1.0 / textureSize(shadow_map, 0).x;
    vec4 shadow_coords = vert_light_pos / vert_light_pos.w;
    for (int x = -radius; x <= radius; x++) {
        for (int y = -radius; y <= radius; y++) {
            vec2 offset = vec2(x, y) * texelSize;
            if (texture(shadow_map, shadow_coords.xy + offset).r < shadow_coords.z - 0.0005){
                shadow -= 1;
            }
        }
    }
    shadow /= shaow_texel_count;

    vec3 final_color = (direct_luminance * shadow) + ambient_contribution + emissive;
    final_color = ACESFilm(final_color);

    out_color = vec4(final_color, albedo.a);
    //    out_color = vec4(vec3(ambient_contribution), albedo.a);
}
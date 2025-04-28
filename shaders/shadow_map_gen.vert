#version 450
#extension GL_ARB_shading_language_include: enable
#include "common.glsl"

layout (location = 0) out vec4 vert_olor;

void main() {
    Vertex v = constants.vertex_buffer.vertices[gl_VertexIndex];
    vec4 vert_position = constants.model_transform * vec4(v.position.xyz, 1.f);
    gl_Position = scene_data.proj * scene_data.view * vert_position;
}

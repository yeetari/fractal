#version 450

layout (push_constant) uniform ObjectData {
    vec2 scale;
    vec2 translation;
} g_object;

layout (location = 0) out vec2 g_uv;

void main() {
    vec2 vertices[6] = vec2[6](
        vec2(-1.0f, -1.0f),
        vec2(1.0f, -1.0f),
        vec2(1.0f, 1.0f),
        vec2(-1.0f, -1.0f),
        vec2(-1.0f, 1.0f),
        vec2(1.0f, 1.0f)
    );
    vec2 position = vertices[gl_VertexIndex] * g_object.scale + (g_object.translation * 2.0f - 1.0f);
    vec2 uv = (vertices[gl_VertexIndex] + 1.0f) * 0.5f;
    gl_Position = vec4(position, 0.0f, 1.0f);
    g_uv = uv / 6.0f + vec2(1.0f / 6.0f, 0.0f);
}

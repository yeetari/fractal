#version 450

struct ObjectData {
    vec2 position;
    vec2 scale;
    vec2 sprite_cell;
};

layout (binding = 0) readonly buffer ObjectDatas {
    ObjectData g_objects[];
};

layout (location = 0) out vec2 g_uv;

const vec2 k_vertices[6] = vec2[6](
    vec2(-1.0f, -1.0f),
    vec2(1.0f, -1.0f),
    vec2(1.0f, 1.0f),
    vec2(-1.0f, -1.0f),
    vec2(-1.0f, 1.0f),
    vec2(1.0f, 1.0f)
);

void main() {
    ObjectData object = g_objects[gl_InstanceIndex];
    vec2 position = k_vertices[gl_VertexIndex] * object.scale + (object.position * 2.0f - 1.0f);
    vec2 uv = (k_vertices[gl_VertexIndex] + 1.0f) * 0.5f;
    gl_Position = vec4(position, 0.0f, 1.0f);
    g_uv = (uv + object.sprite_cell) / 6.0f;
}

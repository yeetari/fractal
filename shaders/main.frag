#version 450

layout (location = 0) in vec2 g_uv;

layout (binding = 1) uniform sampler2D g_sprite_atlas;

layout (location = 0) out vec4 g_out_colour;

void main() {
    g_out_colour = vec4(texture(g_sprite_atlas, g_uv));
}

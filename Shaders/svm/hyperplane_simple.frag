#version 460
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) out vec4 fragColor;

layout(location = 0) in vec2 in_texturecoord;
layout(location = 1) in vec4 in_color;

void main() { fragColor = in_color * vec4(in_texturecoord * 1.0, 1,1); }
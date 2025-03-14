#version 460
#extension GL_ARB_separate_shader_objects : enable
/*  */
layout(location = 0) out vec4 fragColor;
/*  */
layout(location = 0) in vec2 UV;

layout(binding = 1) uniform sampler2D DiffuseTexture;

void main() { fragColor = texture(DiffuseTexture, UV); }
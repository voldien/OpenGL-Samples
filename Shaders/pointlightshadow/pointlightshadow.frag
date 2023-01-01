#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) out float fragColor;
layout(binding = 0) uniform sampler2D DiffuseTexture;

void main() { fragColor = gl_FragCoord.z; }
#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) out vec4 outColor;

layout(location = 0) in vec2 uv;

layout(binding = 0) uniform sampler2D WorldTexture;
layout(binding = 1) uniform sampler2D DepthTexture;
layout(binding = 2) uniform sampler2D NormalTexture;
layout(binding = 3) uniform sampler2D NormalRandomize;

void main() { outColor = vec4(1,1,1, 1.0); }

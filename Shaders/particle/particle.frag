

#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) out vec4 outColor;
layout(location = 1) in vec4 velocity;

layout(binding = 1) uniform sampler2D texSampler;

void main() { outColor = vec4(0, 1, velocity.x, 1.0); }

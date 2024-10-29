#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_include : enable
#extension GL_GOOGLE_include_directive : enable

layout(location = 0) out vec4 fragColor;
layout(location = 0) smooth in vec4 vColor;

#include"base.glsl"

vec4 computeColor() { return (ubo.color + vColor); }

void main() { fragColor = computeColor(); }

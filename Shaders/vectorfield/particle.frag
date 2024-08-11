#version 460
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) out vec4 fragColor;

layout(location = 0) smooth in vec2 uv;
layout(location = 1) smooth in vec4 gColor;
layout(location = 2) in float ageTime;

/*  */
layout(binding = 0) uniform sampler2D spriteTexture;

#include "base.glsl"

void main() { fragColor = texture(spriteTexture, uv) * (ubo.color + gColor); }

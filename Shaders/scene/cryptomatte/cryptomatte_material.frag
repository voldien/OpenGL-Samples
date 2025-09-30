#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_include : enable
#extension GL_GOOGLE_include_directive : enable

precision mediump float;
precision mediump int;

layout(location = 0) out vec4 fragColor;

layout(location = 8) flat in ivec2 fAssigns;

#include "common_frag.glsl"
#include "noise.glsl"

void main() {
	const int material_index = fAssigns.x;

	const vec4 crypto_color = vec4(abs(rand(vec2(material_index, 0))), 0, abs(rand(vec2(material_index, 10))), 1);
	fragColor = crypto_color;
}
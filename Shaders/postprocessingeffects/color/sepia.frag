#version 460
#extension GL_ARB_separate_shader_objects : enable

precision mediump float;
precision mediump int;

layout(location = 0) out vec4 fragColor;
layout(location = 0) in vec2 screenUV;

layout(binding = 1) uniform sampler2D ColorTexture;

float _Blend;

layout(push_constant) uniform Settings {
	layout(offset = 0) float blend;
	layout(offset = 4) float exposure;
	layout(offset = 8) float gamma;
}
settings;

vec4 FragSepia() {
	vec4 color = texture(ColorTexture, screenUV, 0);
	vec3 luminance = vec3(dot(color.rgb, vec3(0.299, 0.587, 0.114)));

	luminance *= vec3(1.0, 0.95, 0.82);

	color.rgb = mix(color.rgb, luminance, _Blend.xxx);
	return vec4(color.rgb, 1.0);
}

vec4 FragBlendWeightsSepia() {
	vec4 color = texture(ColorTexture, screenUV, 0);
	vec3 luminance = vec3(dot(color.rgb, vec3(0.299, 0.587, 0.114)));

	luminance *= vec3(1.2, 1.0, 0.8);

	color.rgb = mix(color.rgb, luminance, _Blend.xxx);
	return vec4(color.rgb, 1.0);
}

void main() { fragColor = FragBlendWeightsSepia(); }
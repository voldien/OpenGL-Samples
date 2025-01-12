#version 460
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) out vec4 fragColor;
layout(location = 0) in vec2 uv;

layout(binding = 1) uniform sampler2D ColorTexture;

float _Blend;

vec4 FragSepia() {
	vec4 color = texture(ColorTexture, uv, 0);
	vec3 luminance = vec3(dot(color.rgb, vec3(0.299, 0.587, 0.114)));

	luminance *= vec3(1.0, 0.95, 0.82);

	color.rgb = mix(color.rgb, luminance, _Blend.xxx);
	return vec4(color.rgb, 1.0);
}

vec4 FragBlendWeightsSepia() {
	vec4 color = texture(ColorTexture, uv, 0);
	vec3 luminance = vec3(dot(color.rgb, vec3(0.299, 0.587, 0.114)));

	luminance *= vec3(1.2, 1.0, 0.8);

	color.rgb = mix(color.rgb, luminance, _Blend.xxx);
	return vec4(color.rgb, 1.0);
}

void main() { fragColor = FragBlendWeightsSepia(); }
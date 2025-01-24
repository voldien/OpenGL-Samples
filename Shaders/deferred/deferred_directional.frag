#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_include : enable
#extension GL_GOOGLE_include_directive : enable

layout(location = 0) out vec4 fragColor;

layout(location = 1) in flat int InstanceID;

layout(binding = 0) uniform sampler2D AlbedoTexture;
layout(binding = 1) uniform sampler2D WorldTexture;
layout(binding = 2) uniform sampler2D DepthTexture;
//layout(binding = 3) uniform sampler2D NormalTexture;

#include"deferred_base.glsl"

//
layout(set = 0, binding = 1, std140) uniform UniformBufferLight { DirectionalLight light[64]; }
directionalLightUBO;

vec2 CalcTexCoord() { return gl_FragCoord.xy / vec2(2560, 1440); }

void main() {

	const vec2 uv = CalcTexCoord();

	const vec4 color = vec4(texture(AlbedoTexture, uv).rgb, 1);
	const vec3 normal = normalize(texture(NormalTexture, uv).xyz);

	const float contribution = max(dot(normal, -normalize(vec3(1, -1, 0))), 0.0);

	const vec4 ambient = vec4(0.3);
	const vec4 lightColor = contribution * vec4(2.7);

	fragColor = color * (ambient + lightColor); // * pointLightColors;
	fragColor.a = 1;
}
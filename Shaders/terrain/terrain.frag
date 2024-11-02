#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_include : enable
#extension GL_GOOGLE_include_directive : enable

layout(location = 0) out vec4 fragColor;

layout(location = 0) in vec3 in_WorldPosition;
layout(location = 1) in vec2 UV;
layout(location = 2) in vec3 normal;
layout(location = 3) in vec3 tangent;

#include "common.glsl"
#include "terrain_base.glsl"
#include"phongblinn.glsl"

layout(binding = 1) uniform sampler2D DiffuseTexture;
layout(binding = 2) uniform sampler2D NormalTexture;
layout(binding = 2) uniform sampler2D Irradiance;

void main() {

	const vec3 alteredNormal = normal;

	// Compute directional light
	vec4 lightColor = computeLightContributionFactor(ubo.direction.xyz, alteredNormal) * ubo.lightColor;

	fragColor = texture(DiffuseTexture, UV);
	fragColor = vec4(UV, 0, 1);
}
 
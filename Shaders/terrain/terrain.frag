#version 460
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) out vec4 fragColor;

layout(location = 0) in vec2 UV;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec3 tangent;

#include "common.glsl"
#include "terrain_base.glsl"

layout(binding = 1) uniform sampler2D DiffuseTexture;
layout(binding = 2) uniform sampler2D NormalTexture;

void main() {

	// Compute directional light
//	vec4 lightColor = computeLightContributionFactor(ubo.direction.xyz, alteredNormal) * ubo.lightColor;

	fragColor = texture(DiffuseTexture, UV);
}
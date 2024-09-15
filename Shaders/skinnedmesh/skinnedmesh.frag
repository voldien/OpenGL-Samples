#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_explicit_attrib_location : enable
#extension GL_ARB_uniform_buffer_object : enable

layout(location = 0) out vec4 fragColor;

layout(location = 0) in vec3 FragIN_position;
layout(location = 1) in vec2 FragIN_uv;
layout(location = 2) in vec3 FragIN_normal;
layout(location = 3) in vec3 FragIN_tangent;

#include "skinnedmesh_common.glsl"

layout(binding = 0) uniform sampler2D DiffuseTexture;

void main() {
	/*	Compute directional light	*/
	const vec4 lightColor = computeLightContributionFactor(ubo.direction.xyz, FragIN_normal) * ubo.lightColor;

	/*	*/
	fragColor = texture(DiffuseTexture, FragIN_uv) * ubo.tintColor * (ubo.ambientColor + lightColor);
}
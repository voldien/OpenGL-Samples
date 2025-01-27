#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_explicit_attrib_location : enable
#extension GL_ARB_uniform_buffer_object : enable
#extension GL_ARB_shading_language_include : enable
#extension GL_GOOGLE_include_directive : enable

layout(location = 0) out vec4 fragColor;

layout(location = 0) in vec3 FragIN_position;
layout(location = 1) in vec2 FragIN_uv;
layout(location = 2) in vec3 FragIN_normal;
layout(location = 3) in vec3 FragIN_tangent;

#include "skinnedmesh_common.glsl"


void main() {

	const material mat = getMaterial();
	const global_rendering_settings glob_settings = constantCommon.constant.globalSettings;

	/*	Compute normal.	*/

	/*	Compute directional light	*/
	const vec4 lightColor =
		computeLightContributionFactor(ubo.directional.direction.xyz, FragIN_normal) * ubo.directional.lightColor;

	/*	*/
	fragColor = (texture(DiffuseTexture, FragIN_uv) * mat.diffuseColor) * (glob_settings.ambientColor * mat.ambientColor + lightColor);
}
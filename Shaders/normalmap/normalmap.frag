#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_explicit_attrib_location : enable
#extension GL_ARB_uniform_buffer_object : enable
#extension GL_ARB_shading_language_include : enable
#extension GL_GOOGLE_include_directive : enable

layout(location = 0) out vec4 fragColor;

layout(location = 0) in vec2 FragIN_uv;
layout(location = 1) in vec3 FragIN_normal;
layout(location = 2) in vec3 FragIN_tangent;
layout(location = 3) in vec3 FragIN_bitangent;

#include "common.glsl"
#include "pbr.glsl"
#include "phongblinn.glsl"
#include "scene.glsl"

layout(binding = 0, std140) uniform UniformBufferBlock {
	mat4 model;
	mat4 view;
	mat4 proj;
	mat4 modelView;
	mat4 ViewProj;
	mat4 modelViewProjection;

	/*	Material Settings.	*/
	vec4 tintColor;

	/*	Light source.	*/
	vec4 direction;
	vec4 lightColor;
	vec4 ambientColor;
	vec4 viewDir;

	/*	*/
	float normalStrength;
	float shininess;
}
ubo;

void main() {

	const material mat = getMaterial();
	const global_rendering_settings glob_settings = constantCommon.constant.globalSettings;

	/*	Convert normal map texture to a vector.	*/
	vec3 NormalMapBump = 2.0 * texture(NormalTexture, FragIN_uv).xyz - vec3(1.0, 1.0, 1.0);

	/*	Scale non forward axis.	*/
	NormalMapBump.xy *= ubo.normalStrength;

	/*	Compute the new normal vector on the specific surface normal.	*/
	const vec3 alteredNormal = normalize(mat3(FragIN_tangent, FragIN_bitangent, FragIN_normal) * NormalMapBump);

	/*	Compute directional light	*/
	const float contribution = computeLightContributionFactor(ubo.direction.xyz, alteredNormal);
	const vec4 lightColor = contribution * ubo.lightColor;

	const vec4 specularColor = contribution * ubo.lightColor;

	/*  Blinn	*/
	const vec3 halfwayDir = normalize(-normalize(ubo.direction.xyz) + ubo.viewDir.xyz);
	const float spec = pow(max(dot(alteredNormal, halfwayDir), 0.0), ubo.shininess);

	/*	*/
	fragColor = (texture(DiffuseTexture, FragIN_uv) * mat.diffuseColor) *
				(mat.ambientColor * glob_settings.ambientColor + lightColor + specularColor * spec);
}
#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_explicit_attrib_location : enable
#extension GL_ARB_uniform_buffer_object : enable

layout(location = 0) out vec4 fragColor;

layout(location = 0) in vec2 FragIN_uv;
layout(location = 1) in vec3 FragIN_normal;
layout(location = 2) in vec3 FragIN_tangent;
layout(location = 3) in vec3 FragIN_bitangent;

layout(binding = 0, std140) uniform UniformBufferBlock {
	mat4 model;
	mat4 view;
	mat4 proj;
	mat4 modelView;
	mat4 ViewProj;
	mat4 modelViewProjection;

	/*	*/
	vec4 tintColor;

	/*	Light source.	*/
	vec4 direction;
	vec4 lightColor;
	vec4 ambientColor;

	/*	*/
	float normalStrength;
}
ubo;

layout(binding = 1) uniform sampler2D DiffuseTexture;
layout(binding = 2) uniform sampler2D NormalTexture;

float computeLightContributionFactor(in const vec3 direction, in const vec3 normalInput) {
	return max(0.0, dot(normalInput, -normalize(direction)));
}

void main() {

	/*	Convert normal map texture to a vector.	*/
	vec3 NormalMapBump = 2.0 * texture(NormalTexture, FragIN_uv).xyz - vec3(1.0, 1.0, 1.0);

	/*	Scale non forward axis.	*/
	NormalMapBump.xy *= ubo.normalStrength;

	/*	Compute the new normal vector on the specific surface normal.	*/
	const vec3 alteredNormal = normalize(mat3(FragIN_tangent, FragIN_bitangent, FragIN_normal) * NormalMapBump);

	/*	Compute directional light	*/
	vec4 lightColor = computeLightContributionFactor(ubo.direction.xyz, alteredNormal) * ubo.lightColor;

	/*	*/
	fragColor = texture(DiffuseTexture, FragIN_uv) * ubo.tintColor * (ubo.ambientColor + lightColor);
}
#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_explicit_attrib_location : enable
#extension GL_ARB_uniform_buffer_object : enable

layout(location = 0) out vec4 fragColor;

layout(location = 0) in vec2 uv;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec3 tangent;

layout(binding = 0, std140) uniform UniformBufferBlock {
	mat4 model;
	mat4 view;
	mat4 proj;
	mat4 modelView;
	mat4 ViewProj;
	mat4 modelViewProjection;

	vec4 tintColor;

	/*	Light source.	*/
	vec4 direction;
	vec4 lightColor;
	vec4 ambientColor;

	float normalStrength;
}
ubo;

layout(binding = 1) uniform sampler2D DiffuseTexture;
layout(binding = 2) uniform sampler2D NormalTexture;

float computeLightContributionFactor(in const vec3 direction, in const vec3 normalInput) {
	return max(0.0, dot(normalInput, -normalize(direction)));
}

void main() {

	/*	Create Surface point matrix bias, needed to map the normal map vector to the world space.	*/
	vec3 Mnormal = normalize(normal);
	vec3 Ttangent = normalize(tangent);
	Ttangent = normalize(Ttangent - dot(Ttangent, Mnormal) * Mnormal);
	vec3 bittagnet = cross(Ttangent, Mnormal);

	/*	Convert normal map texture to a vector.	*/
	vec3 NormalMapBump = 2.0 * texture(NormalTexture, uv).xyz - vec3(1.0, 1.0, 1.0);

	/*	Scale non forward axis.	*/
	NormalMapBump.xy *= ubo.normalStrength;

	/*	Compute the new normal vector on the specific surface normal.	*/
	vec3 alteredNormal = normalize(mat3(Ttangent, bittagnet, Mnormal) * NormalMapBump);

	/*	Compute directional light	*/
	vec4 lightColor = computeLightContributionFactor(ubo.direction.xyz, alteredNormal) * ubo.lightColor;

	fragColor = texture(DiffuseTexture, uv) * ubo.tintColor * (ubo.ambientColor + lightColor);
}
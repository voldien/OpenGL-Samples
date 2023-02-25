#version 460
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) out vec4 fragColor;

layout(location = 0) in vec3 vertex;
layout(location = 1) in vec2 UV;
layout(location = 2) in vec3 normal;
layout(location = 3) in vec3 tangent;
layout(location = 4) in vec3 FragIN_bitangent;

layout(binding = 0) uniform sampler2D DiffuseTexture;
layout(binding = 1) uniform sampler2D NormalTexture;
layout(binding = 2) uniform sampler2D DisplacementTexture;

layout(binding = 0, std140) uniform UniformBufferBlock {
	mat4 model;
	mat4 view;
	mat4 proj;
	mat4 modelView;
	mat4 ViewProj;
	mat4 modelViewProjection;

	/*	Light source.	*/
	vec4 direction;
	vec4 lightColor;
	vec4 ambientColor;
	/*	*/
	vec3 CameraEye;
	float DisplacementHeight;
	float normalStrength;
}
ubo;

vec2 getParallaxMappingOffset(const in sampler2D heightMap, const in vec2 uv, const in vec3 viewDir,
							  const in float height_scale) {
	float height = texture(heightMap, uv).r;

	vec2 p = (viewDir.xy / viewDir.z) * (height * height_scale);

	return uv - p;
}

void main() {

	/*	Compute the new normal vector on the specific surface normal.	*/
	mat3 bias = mat3(tangent, FragIN_bitangent, normal);

	const vec3 cameraDir = normalize((bias * vertex) - (bias * ubo.CameraEye));

	const vec2 texCoords = getParallaxMappingOffset(DisplacementTexture, UV, cameraDir, ubo.DisplacementHeight);
	// if (texCoords.x > 1.0 || texCoords.y > 1.0 || texCoords.x < 0.0 || texCoords.y < 0.0)
	//	discard;

	/*	Convert normal map texture to a vector.	*/
	vec3 NormalMapBump = 2.0 * texture(NormalTexture, texCoords).xyz - vec3(1.0, 1.0, 1.0);

	/*	Scale non forward axis.	*/
	NormalMapBump.xy *= ubo.normalStrength;

	const vec3 alteredNormal = normalize(bias * NormalMapBump);

	vec4 lightColor = max(0.0, dot(alteredNormal, -normalize(ubo.direction.xyz))) * ubo.lightColor;

	fragColor = texture(DiffuseTexture, texCoords) * (ubo.ambientColor + lightColor);
}
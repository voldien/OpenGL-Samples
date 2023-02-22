#version 460
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) out vec4 fragColor;

layout(location = 0) in vec3 vertex;
layout(location = 1) in vec2 UV;
layout(location = 2) in vec3 normal;
layout(location = 3) in vec3 tangent;

layout(binding = 0) uniform sampler2D DiffuseTexture;
layout(binding = 1) uniform sampler2D DisplacementTexture;

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
}
ubo;

vec2 getParallaxMappingOffset(const in sampler2D heightMap, const in vec2 uv, const in vec3 viewDir,
							  const in float height_scale) {
	float height = texture(heightMap, uv).r;
	vec2 p = (viewDir.xy / viewDir.z) * (height * height_scale);
	return uv - p;
}

void main() {
	const vec3 cameraDir = normalize(vertex - ubo.CameraEye);
	const vec2 texCoords = getParallaxMappingOffset(DisplacementTexture, UV, cameraDir, ubo.DisplacementHeight);
	fragColor = texture(DiffuseTexture, texCoords) * (ubo.ambientColor);
}
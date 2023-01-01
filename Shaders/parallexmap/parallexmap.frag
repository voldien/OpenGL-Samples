#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) out vec4 fragColor;

layout(location = 0) in vec2 uv;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec3 tangent;

// smooth in vec3 vTangent;
// smooth in vec3 vCameraEye;
// uniform vec4 DiffuseColor;
// uniform vec2 DisplacementHeight;

layout(binding = 0) uniform sampler2D DiffuseTexture;
layout(binding = 1) uniform sampler2D DisplacementTexture;

layout(binding = 0, std140) uniform UniformBufferBlock {
	mat4 model;
	mat4 view;
	mat4 proj;
	mat4 modelView;
	mat4 modelViewProjection;

	/*	Light source.	*/
	vec4 direction;
	vec4 lightColor;
	vec4 ambientColor;
	vec3 CameraEye;
	vec2 DisplacementHeight;
}
ubo;

vec2 getParallax(const in sampler2D heightMap, const in vec2 uv, const in vec3 cameraDir, const in vec2 biasScale) {
	float v = texture(heightMap, uv).r * biasScale.x - biasScale.y;
	return (uv + (cameraDir.xy * v)).xy;
}

void main() {
	fragColor = texture(DiffuseTexture,
						getParallax(DisplacementTexture, uv, normalize(ubo.CameraEye), ubo.DisplacementHeight)) *
				(ubo.ambientColor);
}
#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_include : enable
#extension GL_GOOGLE_include_directive : enable

layout(location = 0) out vec4 fragColor;

layout(location = 0) in vec2 uv;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec3 tangent;

#include "common.glsl"
#include"scene.glsl"

// layout(binding = 0, std140) uniform UniformBufferBlock {
// 	mat4 model;
// 	mat4 view;
// 	mat4 proj;
// 	mat4 modelView;
// 	mat4 modelViewProjection;

// 	/*	Light source.	*/
// 	vec4 direction;
// 	vec4 lightColor;
// 	vec4 ambientColor;

// 	/*  Fog Settings.   */
// 	vec4 fogColor;
// 	float CameraNear;
// 	float CameraFar;
// 	float fogStart;
// 	float fogEnd;
// 	float fogDensity;

// 	uint fogType;
// 	float fogItensity;
// }
// ubo;

void main() {

	const material mat = MaterialUBO.materials[0];
	//float fogFactor = getFogFactor() * ubo.fogItensity;

	//float contribution = max(0.0, dot(normalize(normal), normalize(ubo.direction.xyz)));

	fragColor = (texture(DiffuseTexture, uv) * mat.diffuseColor) * (mat.ambientColor);
	fragColor.a *= texture(AlphaMaskedTexture, uv).r;
	if (fragColor.a < mat.clip_.x) {
		discard;
	}

	//fragColor = mix(fragColor, ubo.fogColor, fogFactor).rgba;
}
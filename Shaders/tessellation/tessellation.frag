#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_include : enable
#extension GL_GOOGLE_include_directive : enable

/*  */
layout(location = 0) out vec4 fragColor;

/*  */
layout(location = 0) in vec3 VertexPosition;
layout(location = 1) in vec2 UV;
layout(location = 2) in vec3 normal;

#include "phongblinn.glsl"

layout(binding = 0, std140) uniform UniformBufferBlock {
	mat4 model;
	mat4 view;
	mat4 proj;
	mat4 modelView;
	mat4 modelViewProjection;

	/*	Light source.	*/
	DirectionalLight directional;

	/*	Material	*/
	vec4 specularColor;
	vec4 ambientColor;
	vec4 shininess;

	/*	Tessellation Settings.	*/
	vec4 gEyeWorldPos;
	float gDispFactor;
	float tessLevel;
	float maxTessellation;
	float minTessellation;
	float dist;
}
ubo;

/*  */
layout(binding = 0) uniform sampler2D DiffuseTexture;
layout(binding = 1) uniform sampler2D DisplacementTexture;

void main() {

	const vec4 lightColor = computeBlinnDirectional(ubo.directional, normalize(normal), ubo.shininess.yzw,
													ubo.shininess.r, ubo.specularColor.rgb);

	fragColor = texture(DiffuseTexture, UV) * (ubo.ambientColor + lightColor);

	// fragColor = vec4(normal, 1);
}
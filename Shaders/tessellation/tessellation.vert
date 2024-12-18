#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_include : enable
#extension GL_GOOGLE_include_directive : enable

layout(location = 0) in vec3 Vertex;
layout(location = 1) in vec2 TextureCoord;
layout(location = 2) in vec3 Normal;
layout(location = 3) in vec3 Tangent;

layout(location = 0) out vec3 VertexPosition;
layout(location = 1) out vec2 UV;
layout(location = 2) out vec3 normal;
layout(location = 3) out vec3 tangent;

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

void main() {
	VertexPosition = (ubo.model * vec4(Vertex, 1.0)).xyz;
	UV = TextureCoord * 2;
	normal = normalize(ubo.model * vec4(Normal, 0.0)).xyz;
	tangent = normalize(ubo.model * vec4(Tangent, 0.0)).xyz;
}


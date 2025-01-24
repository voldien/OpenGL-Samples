#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_include : enable
#extension GL_GOOGLE_include_directive : enable

layout(location = 0) in vec3 Vertex;
layout(location = 1) in vec2 TextureCoord;
layout(location = 2) in vec3 Normal;
layout(location = 3) in vec3 Tangent;

layout(location = 0) out vec2 UV;
layout(location = 1) out vec3 normal;
layout(location = 2) out vec3 tangent;

#include"common.glsl"
#include"scene.glsl"

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
	vec4 fogColor;

	float CameraNear;
	float CameraFar;
	float fogStart;
	float fogEnd;

	float fogDensity;

	uint fogType;
	float fogItensity;
}
ubo;

void main() {
	
	const mat4 model = NodeUBO.node[gl_InstanceID].model;
	const mat4 viewProj = constantCommon.constant.camera.viewProj;

	gl_Position = (viewProj * model) * vec4(Vertex, 1.0);
	normal = (model * vec4(Normal, 0.0)).xyz;
	tangent = (model * vec4(Tangent, 0.0)).xyz;
	UV = TextureCoord;
}
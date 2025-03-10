#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_explicit_attrib_location : enable
#extension GL_ARB_uniform_buffer_object : enable
#extension GL_ARB_shading_language_include : enable
#extension GL_GOOGLE_include_directive : enable

layout(location = 0) in vec3 Vertex;
layout(location = 1) in vec2 TextureCoord;
layout(location = 8) in ivec2 vAssigns;

layout(location = 0) out vec2 UV;

#include "common.glsl"
#include "phongblinn.glsl"
#include "scene.glsl"

layout(binding = 0, std140) uniform UniformBufferBlock {
	mat4 lightSpaceMatrix;

	/*	Light source.	*/
	DirectionalLight directional;
	float bias;
	float shadowStrength;
	float radius;
}
ubo;

void main() {
	const mat4 model = getModel(vAssigns.y);

	gl_Position = ubo.lightSpaceMatrix * model * vec4(Vertex, 1.0);
	UV = TextureCoord;
}
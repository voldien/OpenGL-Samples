#version 460
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) out vec4 fragColor;
layout(location = 0) in vec3 vVertex;

layout(binding = 0) uniform samplerCube textureCubeMap;

layout(binding = 0) uniform UniformBufferBlock {
	mat4 proj;
	mat4 modelViewProjection;
	vec4 tintColor;
	float exposure;
}
ubo;

void main() { fragColor = texture(textureCubeMap, vVertex); }
#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) out vec4 fragColor;
layout(location = 0) in vec3 vVertex;

layout(binding = 0) uniform samplerCube texture0;

layout(binding = 0) uniform UniformBufferBlock {
	mat4 MVP;
	float exposure;
}
ubo;

void main() { fragColor = texture(texture0, vVertex); }
#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_explicit_attrib_location : enable
#extension GL_ARB_uniform_buffer_object : enable

precision mediump float;
precision mediump int;

layout(location = 0) out vec4 fragColor;

layout(binding = 0, std140) uniform UniformBufferBlock {
	mat4 model;
	mat4 viewProjection;
	mat4 shadowProject;
	vec4 color;
}
ubo;

void main() { fragColor = ubo.color; }
#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_explicit_attrib_location : enable
#extension GL_ARB_uniform_buffer_object : enable

layout(location = 0) in vec3 Vertex;
layout(location = 1) in vec2 TextureCoord;
layout(location = 2) in vec3 Normal;
layout(location = 3) in vec3 Tangent;

layout(binding = 0, std140) uniform UniformBufferBlock {
	mat4 model;
	mat4 viewProjection;
	mat4 shadowProject;
	vec4 color;
}
ubo;

void main() { gl_Position = ubo.viewProjection * (ubo.shadowProject * (ubo.model * vec4(Vertex, 1.0))); }
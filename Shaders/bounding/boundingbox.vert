#version 460 core
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_explicit_attrib_location : enable
#extension GL_ARB_uniform_buffer_object : enable

layout(location = 0) in vec3 Vertex;

layout(location = 3) out vec4 instanceColor;

layout(binding = 0, std140) uniform UniformBufferBlock {
	mat4 model;
	mat4 view;
	mat4 proj;
	mat4 modelView;
	mat4 ViewProj;
	mat4 modelViewProjection;
}
ubo;

layout(binding = 1, std140) uniform UniformInstanceBlock { mat4 model[512]; }
instance_ubo;

float rand(const in vec2 co) { return abs(fract(sin(dot(co.xy, vec2(12.9898, 78.233))) * 43758.5453)); }

void main() {
	gl_Position = ubo.proj * ubo.view * (instance_ubo.model[gl_InstanceID]) * vec4(Vertex, 1.0);

	instanceColor = vec4(abs(rand(vec2(gl_InstanceID, 0))), 0, abs(rand(vec2(gl_InstanceID, 10))), 0.075);
}

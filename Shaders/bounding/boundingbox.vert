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

struct InstanceData{
	mat4 model;
	vec4 color;
};

layout(binding = 1, std140) uniform UniformInstanceBlock {
	InstanceData ins[512];
}
instance_ubo;

void main() {
	gl_Position = ubo.proj * ubo.view * instance_ubo.ins[gl_InstanceID].model * vec4(Vertex, 1.0);
	instanceColor = instance_ubo.ins[gl_InstanceID].color;
}
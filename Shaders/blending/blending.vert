#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_explicit_attrib_location : enable
#extension GL_ARB_uniform_buffer_object : enable

layout(location = 0) in vec3 Vertex;
layout(location = 1) in vec2 TextureCoord;
layout(location = 2) in vec3 Normal;

layout(location = 0) out vec2 FragIN_uv;
layout(location = 1) out vec3 FragIN_normal;
layout(location = 2) out vec4 FragIN_instanceColor;

layout(binding = 0, std140) uniform UniformBufferBlock {
	mat4 model;
	mat4 view;
	mat4 proj;
	mat4 modelView;
	mat4 ViewProj;
	mat4 modelViewProjection;

	/*	Light source.	*/
	vec4 direction;
	vec4 lightColor;
	vec4 ambientColor;
}
ubo;

layout(binding = 1, std140) uniform UniformInstanceBlock {
	vec4 color[4];
	mat4 model[4];
}
instance_ubo;

void main() {
	gl_Position = ubo.proj * ubo.view * instance_ubo.model[gl_InstanceID] * vec4(Vertex, 1.0);
	FragIN_normal = (instance_ubo.model[gl_InstanceID] * vec4(Normal, 0.0)).xyz;
	FragIN_uv = TextureCoord;
	FragIN_instanceColor = instance_ubo.color[gl_InstanceID];
}
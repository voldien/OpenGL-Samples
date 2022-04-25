#version 450
#extension GL_ARB_separate_shader_objects : enable
layout(location = 0) in vec3 Vertex;

layout(binding = 0) uniform UniformBufferBlock {
	mat4 model;
	mat4 view;
	mat4 proj;
	mat4 modelView;
	mat4 modelViewProjection;
	vec4 diffuse;
	float deltaTime;
}
ubo;

void main() {
	gl_Position = ubo.proj * ubo.view * ubo.model * vec4(Vertex, 1.0);
    
}
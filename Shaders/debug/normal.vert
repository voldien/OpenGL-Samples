
#version 460
#extension GL_ARB_separate_shader_objects : enable

/*	*/
layout(location = 0) in vec3 Vertex;
layout(location = 2) in vec3 Normal;
/*	*/
layout(location = 0) smooth out vec3 normal;

/*	*/
layout(binding = 0, std140) uniform UniformBufferBlock {
	mat4 model;
	mat4 view;
	mat4 proj;
	mat4 modelView;
	mat4 ViewProj;
	mat4 modelViewProjection;
}
ubo;

void main() {
	gl_Position = ubo.modelViewProjection * vec4(Vertex, 1.0);
	normal = normalize((ubo.model * vec4(Normal, 0.0)).xyz);
}
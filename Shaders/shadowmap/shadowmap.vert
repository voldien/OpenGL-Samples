#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 Vertex;
layout(location = 1) in vec2 TextureCoord;
layout(location = 2) in vec3 Normal;
layout(location = 3) in vec3 Tangent;

layout(binding = 0, std140) uniform UniformBufferBlock {
	mat4 model;
	mat4 view;
	mat4 proj;
	mat4 modelView;
	mat4 modelViewProjection;
	/*	Light source.	*/
	vec4 direction;
	vec4 lightColor;
	vec4 ambientColor;
}
ubo;

//layout(binding = 1, std140) uniform UniformModelBufferBlock {
//
//}ss;

void main() {
	gl_Position = ubo.modelViewProjection * vec4(Vertex, 1.0);

}
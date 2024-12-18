#version 460
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) out vec4 normalDistance; /*	normal (xyz), distance (w)	*/
layout(location = 1) out vec4 color;

layout(push_constant) uniform UniformBufferBlock {
	layout(offset = 0) mat4 viewProj;
	layout(offset = 16) vec4 normalDistance;
}
ubo;

void main() {

	gl_Position = vec4(0, 0, 0, 1);
	/*	*/
	normalDistance = ubo.normalDistance;
	color = vec4(gl_VertexID, 1, 1, 1);
}
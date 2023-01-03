#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_explicit_attrib_location : enable
#extension GL_ARB_uniform_buffer_object : enable

layout(location = 0) in vec3 Vertex;
layout(location = 0) out vec2 uv;

void main() {
	gl_Position = vec4(Vertex.x, Vertex.y, 0, 1.0);

	uv = (vec2(Vertex.x, Vertex.y) + vec2(1, 1)) * (1.0 / 2.0);
}

#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 vertex;
layout(location = 1) in vec2 textureCoord;

layout(location = 0) out vec2 uv;

void main() {
	gl_Position = vec4(vertex, 1.0);
	uv = (vertex.xy + vec2(1.0)) * (1.0 / 2.0);
}

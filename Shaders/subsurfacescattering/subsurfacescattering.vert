#version 460
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 Vertex;
layout(location = 1) in vec3 UV;

layout(location = 0) out vec2 uv;

layout(binding = 0, std140) uniform UniformBufferBlock { mat4 MVP; }
ubo;

void main() {
	gl_Position = ubo.MVP * vec4(Vertex, 1.0);
	uv = UV.xy;
}

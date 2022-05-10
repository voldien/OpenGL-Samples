#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec3 inColor;

layout(location = 0) out vec2 uv;

layout(binding = 0) uniform UniformBufferBlock { mat4 MVP; }
ubo;

void main() {
	gl_Position = ubo.MVP * vec4(inPosition, 0.0, 1.0);
	uv = inColor.xy;
}

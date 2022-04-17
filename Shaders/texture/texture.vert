#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec3 inColor;

layout(location = 0) out vec2 uv;

layout(location = 0) uniform mat4 MVP;

void main() {
	gl_Position = MVP * vec4(inPosition, 0.0, 1.0);
	uv = inColor.xy;
}

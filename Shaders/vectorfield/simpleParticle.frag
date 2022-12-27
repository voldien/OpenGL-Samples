#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) out vec4 fragColor;
layout(location = 0) smooth in vec4 vColor;

struct particle_setting {
	float speed;
	float lifetime;
	float gravity;
};

layout(binding = 0) uniform UniformBufferBlock {
	mat4 model;
	mat4 view;
	mat4 proj;
	mat4 modelView;
	mat4 modelViewProjection;

	/*	*/
	float deltaTime;
	float time;
	float zoom;
	vec4 ambientColor;
	vec4 color;

	particle_setting setting;
}
ubo;

vec4 computeColor() { return (ubo.color + vColor) + ubo.ambientColor; }

void main() { fragColor = computeColor(); }

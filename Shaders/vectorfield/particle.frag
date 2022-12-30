#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) out vec4 fragColor;

layout(location = 0) smooth in vec2 uv;
layout(location = 1) smooth in vec4 gColor;
layout(location =2) in float ageTime;

/*  */
layout(binding = 0) uniform sampler2D tex0;

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

vec4 computeColor() { return texture(tex0, uv) * (ubo.color + gColor) + ubo.ambientColor; }

void main() { fragColor = computeColor(); }

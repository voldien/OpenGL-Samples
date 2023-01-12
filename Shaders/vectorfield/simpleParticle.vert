#version 460
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec4 vertex;
/*	*/
layout(location = 0) smooth out vec4 vColor;

#include "base.glsl"

layout(binding = 0) uniform UniformBufferBlock {
	mat4 model;
	mat4 view;
	mat4 proj;
	mat4 modelView;
	mat4 modelViewProjection;

	/*	*/
	float deltaTime;

	particle_setting setting;
	motion_t motion;

	vec4 color;
}
ubo;

void main() {
	gl_Position = ubo.view * vec4(vertex.xy, 0.0, 1.0);

	const vec2 velocity = vertex.zw;
	const float reduce = (1.0 / -10.0);
	const float green = (1.0 / 20.0);
	const float blue = (1.0 / 5.0);

	/*	Compute particle color.	*/
	// const float reduce = (1.0 / 20.0);
	// const float green = (1.0 / 20.0);
	// const float blue = (1.0 / 50.0);

	/*	*/
	vColor = vec4(velocity.x * 10 * reduce * length(velocity), velocity.x * green, length(velocity) * blue, 1.0);
}

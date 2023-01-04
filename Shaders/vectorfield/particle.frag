#version 460
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) out vec4 fragColor;

layout(location = 0) smooth in vec2 uv;
layout(location = 1) smooth in vec4 gColor;
layout(location = 2) in float ageTime;

/*  */
layout(binding = 0) uniform sampler2D tex0;

struct particle_t {
	vec3 position;
	float time;
	vec4 velocity;
};

struct particle_setting {
	float speed;
	float lifetime;
	float gravity;
	float strength;
	float density;
	uvec3 particleBox;
};

/**
 * Motion pointer.
 */
struct motion_t {
	vec2 pos; /*  Position in pixel space.    */
	vec2 velocity /*  direction and magnitude of mouse movement.  */;
	float radius; /*  Radius of incluense, also the pressure of input.    */
};

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

	vec4 ambientColor;
	vec4 color;
}
ubo;

void main() { fragColor = texture(tex0, uv) * (ubo.color + gColor) + ubo.ambientColor; }

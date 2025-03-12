layout(constant_id = 1) const int NR_Particles = 8;

#include "common.glsl"

struct particle_t {
	vec3 position;
	float time;
	vec4 velocity; /*	velocity, mass*/
};

struct vector_field_t {
	vec3 position;
	vec3 force;
};

struct particle_setting {
	uvec4 particleBox;
	uvec4 vectorfieldbox;

	float speed;
	float lifetime;
	float gravity;
	float strength;

	float density;
	uint nrParticles;
	float spriteSize;
	float dragMag;
};

/**
 * Motion pointer.
 */
struct motion_t {
	vec2 pos; /*  Position in pixel space.    */
	vec2 velocity /*  direction and magnitude of mouse movement.  */;
	float radius; /*  Radius of incluense, also the pressure of input.    */
	float amplitude;
	float noise;
	float pad2;
};

layout(binding = 0) uniform UniformBufferBlock {
	mat4 model;
	mat4 view;
	mat4 proj;
	mat4 modelView;
	mat4 modelViewProjection;

	vec4 color;

	particle_setting setting;
	motion_t motion;
	/*	*/
	float deltaTime;
}
ubo;

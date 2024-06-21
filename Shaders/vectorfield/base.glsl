
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
	float pad2;
};

/**
 * Motion pointer.
 */
struct motion_t {
	vec2 pos; /*  Position in pixel space.    */
	vec2 velocity /*  direction and magnitude of mouse movement.  */;
	float radius; /*  Radius of incluense, also the pressure of input.    */
	float amplitude;
	float pad1;
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

vec3 hsv2rgb(vec3 c)
{
    vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}
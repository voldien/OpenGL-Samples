struct particle_t {
	vec3 position;
	float time;
	vec4 velocity;
};

struct particle_setting {
	uvec3 particleBox;
	uvec3 vectorfieldbox;
	float speed;
	float lifetime;
	float gravity;
	float strength;
	float density;
};

/**
 * Motion pointer.
 */
struct motion_t {
	vec2 pos; /*  Position in pixel space.    */
	vec2 velocity /*  direction and magnitude of mouse movement.  */;
	float radius; /*  Radius of incluense, also the pressure of input.    */
};


struct particle_t {
	vec3 position;
	float time;
	vec4 velocity;
};

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

	particle_setting setting;

	/*	*/
	float deltaTime;
}
ubo;
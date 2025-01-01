#include "common.glsl"

struct particle_t {
	vec2 h0;
	vec2 ht_real_img;
};

// complex math functions
vec2 conjugate(vec2 arg) { return vec2(arg.x, -arg.y); }

vec2 complex_exp(float arg) {
	float s;
	float c;
	// s = sincos(arg, &c);
	return vec2(c, s);
}

layout(binding = 0) uniform UniformBufferBlock {
	mat4 model;
	mat4 view;
	mat4 proj;
	mat4 modelView;
	mat4 modelViewProjection;

	/*light source.	*/
	vec4 direction;
	vec4 lightColor;
	vec4 ambientColor;

	float width, height;
	float speed;
	float delta;
	float patchSize;

	vec4 gEyeWorldPos;
	float gDispFactor;
	float tessLevel;
}
ubo;

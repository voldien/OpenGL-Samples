#version 460
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec4 inPosition;
layout(location = 1) in vec4 inVelocity;

layout(location = 0) out vec4 velocity;

struct particle_setting {
	float speed;
	float lifetime;
	float rate;
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

	particle_setting setting;
}
ubo;

const vec3 a = vec3(0, 2, 0);
// acceleration of particles
// vec3 g = vec3(0,-9.8,0); // acceleration due to gravity

// constants
const float PI = 3.14159;
const float TWO_PI = 2 * PI;

// colormap colours
const vec3 RED = vec3(1, 0, 0);
const vec3 GREEN = vec3(0, 1, 0);
const vec3 YELLOW = vec3(1, 1, 0);

// pseudorandom number generator
float rand(vec2 co) { return fract(sin(dot(co.xy, vec2(12.9898, 78.233))) * 43758.5453); }

// pseudorandom direction on a sphere
vec3 uniformRadomDir(vec2 v, out vec2 r) {
	r.x = rand(v.xy);
	r.y = rand(v.yx);
	float theta = mix(0.0, PI / 6.0, r.x);
	float phi = mix(0.0, TWO_PI, r.y);
	return vec3(sin(theta) * cos(phi), cos(theta), sin(theta) * sin(phi));
}

void main() {

	vec3 pos = vec3(0);
	float t = gl_VertexIndex * ubo.setting.rate;
	float alpha = 1;

	if (ubo.time > t) {
		float dt = mod((ubo.time - t), ubo.setting.lifetime);
		vec2 xy = vec2(gl_VertexIndex, t);
		vec2 rdm = vec2(0);
		pos = ((uniformRadomDir(xy, rdm) + 0.5 * a * dt) * dt);
		alpha = 1.0 - (dt / ubo.setting.lifetime);
	}
	vec4 vSmoothColor = vec4(mix(RED, YELLOW, alpha), alpha);
	gl_Position = ubo.modelViewProjection * vec4(pos, 1);
}

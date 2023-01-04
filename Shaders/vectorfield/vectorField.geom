#version 460
#extension GL_ARB_separate_shader_objects : enable

layout(points) in;
layout(line_strip) out;
layout(max_vertices = 6) out;

struct particle_t {
	vec3 position;
	float time;
	vec4 velocity;
};

struct particle_setting {
	uvec3 particleBox;
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

/*  */

layout(location = 0) smooth out vec3 amplitude;

/*	Compute arrow color.	*/
vec3 computeColor(const in vec3 dir) { return vec3((dir.x + 1.0) / 2.0, (dir.y + 1.0) / 2.0, (dir.x - 1.0) / 2.0); }

void main() {

	const float PI = 3.14159265359;
	const float hPI = PI;
	const float arrowAngle = PI / 7.0;
	const vec3 identity = vec3(1.0, 0.0, 0.0);
	const float arrowLength = 1.0 / 3.5;

	/*  Create Billboard quad.    */
	for (int i = 0; i < gl_in.length(); i++) {

		const vec4 glpos = gl_in[i].gl_Position;

		const vec3 pos = glpos.xyz;
		const vec3 dir = glpos.zwx;
		const vec3 endPos = pos + dir;

		/*	Start position.	*/
		gl_Position = ubo.view * vec4(pos, 1.0);
		amplitude = computeColor(dir);
		EmitVertex();

		/*	End position.	*/
		gl_Position = ubo.view * vec4(endPos, 1.0);
		amplitude = computeColor(dir);
		EmitVertex();
		EndPrimitive();

		/*	Start left arrow position.	*/
		gl_Position = ubo.view * vec4(endPos, 1.0);
		amplitude = computeColor(dir);
		EmitVertex();

		/*	Precompute variables.	*/
		const vec3 deltaDir = normalize(endPos - pos);
		const float pgag = dot(deltaDir, identity);
		const float pang = acos(pgag) * sign(deltaDir.y);

		vec3 lvec = vec3(cos(hPI + arrowAngle + pang), sin(hPI + arrowAngle + pang), 0) * length(dir) * arrowLength;
		gl_Position = ubo.view * vec4(endPos + lvec, 1.0);
		EmitVertex();
		EndPrimitive();

		/*	Start right arrow position.	*/
		gl_Position = ubo.view * vec4(endPos, 1.0);
		amplitude = computeColor(dir);
		EmitVertex();

		vec3 rvec = vec3(cos(hPI - arrowAngle + pang), sin(hPI - arrowAngle + pang), 0) * length(dir) * arrowLength;
		gl_Position = ubo.view * vec4(endPos + rvec, 1.0);
		EmitVertex();
		EndPrimitive();
	}
}

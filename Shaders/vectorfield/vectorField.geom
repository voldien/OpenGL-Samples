#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(points) in;
layout(line_strip) out;
layout(max_vertices = 6) out;

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

/*  */

layout(location = 0) smooth out vec3 amplitude;

/*	Compute arrow color.	*/
vec3 computeColor(const in vec2 dir) { return vec3((dir.x + 1.0) / 2.0, (dir.y + 1.0) / 2.0, (dir.x - 1.0) / 2.0); }

void main() {

	const float PI = 3.14159265359;
	const float hPI = PI;
	const float arrowAngle = PI / 7.0;
	const vec2 identity = vec2(1.0, 0.0);
	const float arrowLength = 1.0 / 3.5;

	/*  Create Billboard quad.    */
	for (int i = 0; i < gl_in.length(); i++) {

		const vec4 glpos = gl_in[i].gl_Position;

		const vec2 pos = glpos.xy;
		const vec2 dir = glpos.zw;
		const vec2 endPos = pos + dir;

		/*	Start position.	*/
		gl_Position = ubo.view * vec4(pos, 0.0, 1.0);
		amplitude = computeColor(dir);
		EmitVertex();

		/*	End position.	*/
		gl_Position = ubo.view * vec4(endPos, 0.0, 1.0);
		amplitude = computeColor(dir);
		EmitVertex();
		EndPrimitive();

		/*	Start left arrow position.	*/
		gl_Position = ubo.view * vec4(endPos, 0.0, 1.0);
		amplitude = computeColor(dir);
		EmitVertex();

		/*	Precompute variables.	*/
		const vec2 deltaDir = normalize(endPos - pos);
		const float pgag = dot(deltaDir, identity);
		const float pang = acos(pgag) * sign(deltaDir.y);

		vec2 lvec = vec2(cos(hPI + arrowAngle + pang), sin(hPI + arrowAngle + pang)) * length(dir) * arrowLength;
		gl_Position = ubo.view * vec4(endPos + lvec, 0.0, 1.0);
		EmitVertex();
		EndPrimitive();

		/*	Start right arrow position.	*/
		gl_Position = ubo.view * vec4(endPos, 0.0, 1.0);
		amplitude = computeColor(dir);
		EmitVertex();

		vec2 rvec = vec2(cos(hPI - arrowAngle + pang), sin(hPI - arrowAngle + pang)) * length(dir) * arrowLength;
		gl_Position = ubo.view * vec4(endPos + rvec, 0.0, 1.0);
		EmitVertex();
		EndPrimitive();
	}
}

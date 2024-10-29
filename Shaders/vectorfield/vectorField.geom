#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_include : enable
#extension GL_GOOGLE_include_directive : enable

layout(points) in;
layout(line_strip) out;
layout(max_vertices = 6) out;

#include "base.glsl"


/*  */
layout(location = 0) smooth in vec3 outForce[];

layout(location = 0) smooth out vec3 amplitude;

/*	Compute arrow color.	*/
vec3 computeForceColor(const in vec3 dir) {
	return vec3((dir.x + 1.0) / 2.0, (dir.y + 1.0) / 2.0, (dir.x - 1.0) / 2.0);
}


void main() {

	const float PI = 3.14159265359;
	const float hPI = PI;
	const float arrowAngle = PI / 7.0;
	const vec3 identity = vec3(1.0, 0.0, 0.0);
	const float arrowLength = 1.0 / 3.5;

	for (int i = 0; i < gl_in.length(); i++) {

		const vec4 glpos = gl_in[i].gl_Position;

		const vec3 pos = glpos.xyz;

		const vec3 dir = outForce[i];
		const vec3 endPos = pos + dir;

		/*	Start position.	*/
		gl_Position = ubo.modelViewProjection * vec4(pos, 1.0);
		amplitude = computeForceColor(dir);
		EmitVertex();

		/*	End position.	*/
		gl_Position = ubo.modelViewProjection * vec4(endPos, 1.0);
		amplitude = computeForceColor(dir);
		EmitVertex();
		EndPrimitive();

		/*	Start left arrow position.	*/
		gl_Position = ubo.modelViewProjection * vec4(endPos, 1.0);
		amplitude = computeForceColor(dir);
		EmitVertex();

		/*	Precompute variables.	*/
		const vec3 deltaDir = normalize(endPos - pos);
		const float pgag = dot(deltaDir, identity);
		const float pang = acos(pgag) * sign(deltaDir.y);

		vec3 lvec = vec3(cos(hPI + arrowAngle + pang), sin(hPI + arrowAngle + pang), 0) * length(dir) * arrowLength;
		gl_Position = ubo.modelViewProjection * vec4(endPos + lvec, 1.0);
		EmitVertex();
		EndPrimitive();

		/*	Start right arrow position.	*/
		gl_Position = ubo.modelViewProjection * vec4(endPos, 1.0);
		amplitude = computeForceColor(dir);
		EmitVertex();

		vec3 rvec = vec3(cos(hPI - arrowAngle + pang), sin(hPI - arrowAngle + pang), 0) * length(dir) * arrowLength;
		gl_Position = ubo.modelViewProjection * vec4(endPos + rvec, 1.0);
		EmitVertex();
		EndPrimitive();
	}
}

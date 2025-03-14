#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_explicit_attrib_location : enable
#extension GL_ARB_uniform_buffer_object : enable
#extension GL_EXT_control_flow_attributes : enable

layout(points) in;
layout(triangle_strip, max_vertices = 4) out;

layout(location = 0) out vec2 out_texturecoord;
layout(location = 1) out vec4 out_color;

layout(location = 0) in vec4 normalDistance[]; /*	normal (xyz), distance (w)	*/
layout(location = 1) in vec4 in_color[];	   /*	*/

layout(push_constant) uniform UniformBufferBlock {
	layout(offset = 0) mat4 viewProj;
	layout(offset = 16) vec4 normalDistance;
}
ubo;

void main() {

	/*	*/
	const float plane_distance = 10000;

	/*	*/
	const vec3 hyperplane_up = normalize(normalDistance[0].xyz);
	const vec3 VPosition = hyperplane_up * normalDistance[0].w;

	/*  */
	const vec3 tangent = normalize(cross(hyperplane_up, vec3(1.0, 0.0, 0.0)));
	const vec3 bitangent = normalize(cross(hyperplane_up, tangent));

	const vec2 vertex[4] = {vec2(-1, 1), vec2(1, 1), vec2(-1, -1), vec2(1, -1)};
	[[unroll]] for (uint i = 0; i < 4; i++) {
		/*  */
		vec3 vertexPosition =
			VPosition + (tangent * plane_distance * vertex[i].x) + (bitangent * plane_distance * vertex[i].y);
		gl_Position = ubo.viewProj * vec4(vertexPosition, 1.0);
		out_texturecoord = vertex[i] * 0.5 + 1;
		out_color = in_color[0];
		EmitVertex();
	}

	EndPrimitive();
}
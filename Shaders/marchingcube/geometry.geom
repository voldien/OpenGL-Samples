#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_explicit_attrib_location : enable
#extension GL_ARB_uniform_buffer_object : enable
#extension GL_EXT_control_flow_attributes : enable

layout(triangles) in;
layout(triangle_strip, max_vertices = 3) out;

layout(location = 0) in vec2 in_coord[3];
layout(location = 1) in vec3 in_color[3];
layout(location = 2) in vec3 in_normal_worldspace[3];
layout(location = 3) in float in_scale[3];

layout(location = 0) out vec2 out_coord;
layout(location = 1) out vec3 out_color;
layout(location = 2) out vec3 out_normal_worldspace;

void main() {

	/*  Determine if any visable geometry.  */
	if (in_scale[0] == 0 && in_scale[1] == 0 && in_scale[2] == 0) {
		return;
	}

	[[unroll]] for (uint i = 0; i < 3; i++) {
		gl_Position = gl_in[i].gl_Position;
		out_coord = in_coord[i];
		out_color = in_color[i];
		out_normal_worldspace = in_normal_worldspace[i];
		EmitVertex();
	}

	EndPrimitive();
}
#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_explicit_attrib_location : enable
#extension GL_ARB_uniform_buffer_object : enable
#extension GL_EXT_control_flow_attributes : enable

layout(triangles) in;
layout(triangle_strip, max_vertices = 3) out;

layout(location = 0) in vec3 v_out_vertex_worldspace[];
layout(location = 1) in vec2 v_out_coord[];
layout(location = 2) in vec3 v_out_color[];
layout(location = 3) in vec3 v_out_normal_worldspace[];
layout(location = 4) in float v_out_scale[];

layout(location = 0, xfb_buffer = 0, xfb_offset = 0) out vec3 out_vertex_worldspace;
layout(location = 1, xfb_buffer = 0, xfb_offset = 12) out vec2 out_coord;
layout(location = 2, xfb_buffer = 0, xfb_offset = 20) out vec3 out_color;
layout(location = 3, xfb_buffer = 0, xfb_offset = 32) out vec3 out_normal_worldspace;

void main() {

	/*  Determine if any visable geometry.  */
	if (v_out_scale[0] < 0.001 && v_out_scale[1] < 0.001 && v_out_scale[2] < 0.001) {
		return;
	}

	[[unroll]] for (uint i = 0; i < 3; i++) {
		//gl_Position = vec4(v_out_vertex_worldspace[i].xyz, 1);
		out_vertex_worldspace = v_out_vertex_worldspace[i].xyz;
		out_coord = v_out_coord[i];
		out_color = v_out_color[i];
		out_normal_worldspace = v_out_normal_worldspace[i];
		EmitVertex();
	}
	
	EndPrimitive();
}
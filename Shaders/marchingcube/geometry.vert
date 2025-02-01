#version 460 core
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_include : enable
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_control_flow_attributes : enable

layout(location = 0) out vec3 v_out_vertex_worldspace;
layout(location = 1) out vec2 v_out_coord;
layout(location = 2) out vec3 v_out_color;
layout(location = 3) out vec3 v_out_normal_worldspace;
layout(location = 4) out float v_out_scale;

#include "marchinbcube_common.glsl"

layout(std430, set = 0, binding = 1) buffer readonly GeomBuffer { Vertex vertices[]; };

void main() {
	const int triID = gl_VertexID / 3;

	const vec4 vertexPosition = vec4(vertices[gl_VertexID].pos, 1);

	v_out_color = vertices[gl_VertexID].normal;

	v_out_normal_worldspace = vertices[gl_VertexID].normal;
	v_out_scale = vertices[gl_VertexID].scale;
	v_out_coord = vec2(0);

	v_out_vertex_worldspace = vertexPosition.xyz;
}
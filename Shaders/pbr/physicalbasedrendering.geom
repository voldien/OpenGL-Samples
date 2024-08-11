#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_explicit_attrib_location : enable
#extension GL_ARB_uniform_buffer_object : enable
#extension GL_EXT_control_flow_attributes : enable

layout(triangles) in;
layout(triangle_strip, max_vertices = 18) out;

layout(location = 0) in flat int GIndex[];

layout(location = 0) out vec4 FragVertex;

layout(constant_id = 0) const float NRFaces = 6;

#include "pbr_common.glsl"

void main() {

	[[unroll]] for (int face = 0; face < NRFaces; ++face) {
		gl_Layer = face;					   // built-in variable that specifies to which face we render.
		[[unroll]] for (int i = 0; i < 3; ++i) // for each triangle vertex
		{
			FragVertex = gl_in[i].gl_Position;
			// FIndex = GIndex[i];
			// gl_Position = (ubo.ViewProjection[gl_Layer]) * gl_in[i].gl_Position;
			EmitVertex();
		}
		EndPrimitive();
	}
}
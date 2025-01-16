#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_explicit_attrib_location : enable
#extension GL_ARB_uniform_buffer_object : enable
#extension GL_EXT_control_flow_attributes : enable
#extension GL_ARB_shading_language_include : enable
#extension GL_GOOGLE_include_directive : enable

layout(triangles) in;
layout(triangle_strip, max_vertices = 18) out;

layout(location = 0) in vec3 InVertex[];
layout(location = 0) out vec3 OutVertex;

layout(constant_id = 8) const int NrLayers = 6;
layout(constant_id = 9) const int NrFaces = 3;

void main() {

	[[unroll]] for (uint face = 0; face < NrLayers; ++face) {
		gl_Layer = int(face);							  // built-in variable that specifies to which face we render.
		[[unroll]] for (uint i = 0; i < NrFaces; ++i) // for each triangle vertex
		{
			OutVertex = gl_in[i].gl_Position.xyz;
			gl_Position = gl_in[i].gl_Position;
			EmitVertex();
		}
		EndPrimitive();
	}
}
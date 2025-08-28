#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_explicit_attrib_location : enable
#extension GL_ARB_uniform_buffer_object : enable
#extension GL_EXT_control_flow_attributes : enable
#extension GL_ARB_shading_language_include : enable
#extension GL_GOOGLE_include_directive : enable

layout(triangles) in;
layout(triangle_strip, max_vertices = 18) out;

layout(location = 0) in flat int GIndex[];
layout(location = 1) in vec2 InTextureCoord[];

layout(location = 0) out vec4 FragVertex;
layout(location = 2) out vec2 FragTextureCoord;
layout(location = 1) out  flat int FIndex; // invariant

#include "scene.glsl"

void main() {

	const PointLight pointLight = getPointLight(FIndex);

	[[unroll]] for (int face = 0; face < 6; ++face) {
		gl_Layer = face;						// built-in variable that specifies to which face we render.
		[[unroll]] for (uint i = 0; i < 3; ++i) // for each triangle vertex
		{
			FragVertex = gl_in[i].gl_Position;
			FIndex = GIndex[i];
			FragTextureCoord = InTextureCoord[i];
			 mat4 viewProj; // TODO: gl_Layer
			gl_Position = viewProj * gl_in[i].gl_Position;
			EmitVertex();
		}
		EndPrimitive();
	}
}
#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_explicit_attrib_location : enable
#extension GL_ARB_uniform_buffer_object : enable
#extension GL_ARB_shading_language_include : enable
#extension GL_GOOGLE_include_directive : enable

layout(location = 0) out vec2 screenUV;

#include "postprocessing_base.glsl"

void main() {

	const vec2 vertex[4] = {vec2(-1, 1), vec2(1, 1), vec2(-1, -1), vec2(1, -1)};

	const vec2 Vertex = vertex[gl_VertexID % 4];

	gl_Position = vec4(Vertex, 0, 1.0);
	screenUV = (vec2(Vertex.x, Vertex.y) + vec2(1, 1)) / 2.0;
}

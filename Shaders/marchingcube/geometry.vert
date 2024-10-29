#version 460 core
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_include : enable
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_control_flow_attributes : enable

layout(location = 0) out vec2 coord;
layout(location = 1) out vec3 color;

layout(location = 2) out vec3 normal_worldspace;
layout(location = 3) out float scale;

#include "marchinbcube_common.glsl"

layout(std430, set = 0, binding = 1) buffer readonly GeomBuffer { Vertex vertices[]; };

void main() {
	const int triID = gl_VertexID / 3;

	vec4 vertexPosition = ubo.modelView * vec4(vertices[gl_VertexID].pos, 1);

	// color = vertices[gl_VertexID].pos;
	color = vertices[gl_VertexID].normal;
	// color = vec3(1, 0.5, 1);
	normal_worldspace = vertices[gl_VertexID].normal;
	scale = vertices[gl_VertexID].scale;

	gl_Position = ubo.proj * vertexPosition;
}
#version 460 core
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_include : enable
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_control_flow_attributes : enable

layout(location = 0) in vec3 out_vertex_worldspace;
layout(location = 1) in vec2 out_coord;
layout(location = 2) in vec3 out_color;
layout(location = 3) in vec3 out_normal_worldspace;

layout(location = 0) out vec2 coord;
layout(location = 1) out vec3 color;
layout(location = 2) out vec3 normal;

#include "marchinbcube_common.glsl"

void main() {

	const vec4 vertexPosition = vec4(out_vertex_worldspace.xyz, 1);

	color = out_color;

	normal = out_normal_worldspace;
//	scale = 1;

	gl_Position = ubo.modelViewProjection * vertexPosition;
}
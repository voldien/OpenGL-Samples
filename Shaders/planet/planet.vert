#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_include : enable
#extension GL_GOOGLE_include_directive : enable

layout(location = 0) in vec3 Vertex;
layout(location = 2) in vec3 Normal;
layout(location = 3) in vec3 Tangent;

layout(location = 0) out smooth vec3 WorldPos_in;
layout(location = 1) out smooth vec2 FragIN_uv;
layout(location = 2) out smooth vec3 FragIN_normal;
layout(location = 3) out smooth vec3 FragIN_tangent;

#include "planet_common.glsl"

void main() {

	WorldPos_in = (ubo.model * vec4(Vertex, 1.0)).xyz;

	/*	*/
	FragIN_uv = sphere_uv_mapping(WorldPos_in);
	
	/*	*/
	FragIN_normal = (ubo.model * vec4(Normal, 0.0)).xyz;
	FragIN_tangent = (ubo.model * vec4(Tangent, 0.0)).xyz;
}
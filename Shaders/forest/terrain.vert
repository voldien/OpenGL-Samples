#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_include : enable
#extension GL_GOOGLE_include_directive : enable

layout(location = 0) in vec3 Vertex;
layout(location = 2) in vec3 Normal;
layout(location = 3) in vec3 Tangent;

layout(location = 0) out vec3 WorldPos_in;
layout(location = 1) out vec2 FragIN_uv;
layout(location = 2) out vec3 FragIN_normal;
layout(location = 3) out vec3 FragIN_tangent;

#include "terrain_base.glsl"

void main() {

	// TODO: compute X,Y from vertex ID.
	const float x = float(gl_VertexID % ubo.terrain.size.x);
	const float y = float(gl_VertexID / ubo.terrain.size.y);

	/*	*/
	const vec2 tile_uv = ubo.terrain.tileOffset;
	
	WorldPos_in = (ubo.model * vec4(Vertex - vec3(0,0,0.8), 1.0)).xyz;
	FragIN_uv = ((Vertex.xy + ubo.terrain.size) / vec2(ubo.terrain.size)) * 0.5;

	FragIN_normal = (ubo.model * vec4(Normal, 0.0)).xyz;
	FragIN_tangent = (ubo.model * vec4(Tangent, 0.0)).xyz;
}

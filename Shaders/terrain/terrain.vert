#version 460
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 Vertex;
layout(location = 1) in vec2 TextureCoord;
layout(location = 2) in vec3 Normal;
layout(location = 3) in vec3 Tangent;


layout(location = 0) out vec3 WorldPos_in;
layout(location = 1) out vec2 FragIN_uv;
layout(location = 2) out vec3 FragIN_normal;
layout(location = 3) out vec3 FragIN_tangent;
layout(location = 4) out vec3 FragIN_bitangent;

#include "common.glsl"
#include "terrain_base.glsl"

void main() {
	
	gl_Position = ubo.model * vec4(Vertex, 1.0);

	FragIN_normal = (ubo.model * vec4(Normal, 0.0)).xyz;
	FragIN_tangent = (ubo.model * vec4(Tangent, 0.0)).xyz;
	FragIN_uv = TextureCoord;

	vec3 Mnormal = normalize(FragIN_normal);
	vec3 Ttangent = normalize(FragIN_tangent);
	
	Ttangent = normalize(Ttangent - dot(Ttangent, Mnormal) * Mnormal);
	FragIN_bitangent = cross(Ttangent, Mnormal);
}


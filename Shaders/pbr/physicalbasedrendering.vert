#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_include : enable

layout(location = 0) in vec3 Vertex;
layout(location = 1) in vec2 TextureCoord;
layout(location = 2) in vec3 Normal;
layout(location = 3) in vec3 Tangent;

layout(location = 0) out vec3 vertex;
layout(location = 1) out vec2 uv;
layout(location = 2) out vec3 normal;
layout(location = 3) out vec3 tangent;

#include "pbr_common.glsl"

void main() {
	gl_Position = ubo.model * vec4(Vertex, 1.0);
	vertex = (ubo.model * vec4(Vertex, 1.0)).xyz;
	normal = (ubo.model * vec4(Normal, 0.0)).xyz;
	tangent = (ubo.model * vec4(Tangent, 0.0)).xyz;
	uv = TextureCoord;
}
#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) out vec3 Diffuse;
layout(location = 1) out vec3 WorldSpace;
layout(location = 2) out vec2 TextureCoord;
layout(location = 3) out vec3 Normal;

layout(location = 0) in vec4 vertex;
layout(location = 1) in vec2 uv;
layout(location = 2) in vec3 normal;
layout(location = 3) in vec3 tangent;

layout(binding = 2) uniform sampler2D DiffuseTexture;

void main() {

	Diffuse = texture(DiffuseTexture, uv).rgb;

	Normal = normalize(normal);
	WorldSpace = vertex.xyz;
	TextureCoord.xy = uv;
}
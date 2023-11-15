#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_explicit_attrib_location : enable
#extension GL_ARB_uniform_buffer_object : enable

layout(location = 0) out vec4 Diffuse;
layout(location = 1) out vec3 WorldSpace;
layout(location = 2) out vec4 TextureCoord;
layout(location = 3) out vec3 Normal;

layout(location = 0) in vec4 vertex;
layout(location = 1) in vec2 uv;
layout(location = 2) in vec3 normal;
layout(location = 3) in vec3 tangent;
layout(location = 4) in vec3 bitangent;

layout(binding = 2) uniform sampler2D DiffuseTexture;
layout(binding = 3) uniform sampler2D NormalTexture;
layout(binding = 4) uniform sampler2D AlphaMaskedTexture;

void main() {

	Diffuse = texture(DiffuseTexture, uv).rgba;
	Diffuse.a *= texture(AlphaMaskedTexture, uv).r;

	WorldSpace = vertex.xyz;
	TextureCoord = vec4(uv, 0, 0);

	/*	Convert normal map texture to a vector.	*/
	const vec3 NormalMapBump = 2.0 * texture(NormalTexture, uv).xyz - 1;

	/*	Compute the new normal vector on the specific surface normal.	*/
	Normal = normalize(mat3(tangent, bitangent, normal) * NormalMapBump);
}
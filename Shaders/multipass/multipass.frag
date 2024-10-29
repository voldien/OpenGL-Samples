#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_explicit_attrib_location : enable
#extension GL_ARB_uniform_buffer_object : enable

/*	*/
layout(location = 0) out vec4 Diffuse;
layout(location = 1) out vec4 WorldSpace;
layout(location = 2) out vec4 TextureCoord;
layout(location = 3) out vec3 Normal;
layout(location = 4) out vec4 Roughness_Metalic;
layout(location = 5) out vec3 Specular;

layout(location = 0) in vec4 vertex;
layout(location = 1) in vec2 uv;
layout(location = 2) in vec3 normal;
layout(location = 3) in vec3 tangent;
layout(location = 4) in vec3 bitangent;

layout(binding = 2) uniform sampler2D DiffuseTexture;
layout(binding = 3) uniform sampler2D NormalTexture;
layout(binding = 4) uniform sampler2D AlphaMaskedTexture;
layout(binding = 5) uniform sampler2D RoughnessTexture;
layout(binding = 6) uniform sampler2D MetalicTexture;
layout(binding = 7) uniform sampler2D EmissionTexture;

void main() {

	/*	*/
	Diffuse = texture(DiffuseTexture, uv).rgba;
	Diffuse.a *= texture(AlphaMaskedTexture, uv).r;

	/*	*/
	WorldSpace = vec4(vertex.xyz, 1);
	TextureCoord = vec4(uv, 0, 0);

	/*	Convert normal map texture to a vector.	*/
	const vec3 NormalMap = (2.0 * texture(NormalTexture, uv).xyz) - vec3(1.0, 1.0, 1.0);

	/*	Compute the new normal vector on the specific surface normal.	*/
	Normal = normalize(mat3(tangent, bitangent, normal) * NormalMap);
}
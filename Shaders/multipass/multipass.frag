#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_explicit_attrib_location : enable
#extension GL_ARB_uniform_buffer_object : enable
#extension GL_ARB_shading_language_include : enable
#extension GL_GOOGLE_include_directive : enable
/*	*/
layout(location = 0) out vec4 Diffuse;
layout(location = 1) out vec4 WorldSpace;
layout(location = 2) out vec4 TextureCoord;
layout(location = 3) out vec4 Normal;
layout(location = 4) out vec3 Specular;
layout(location = 5) out vec3 Roughness_Metalic;
layout(location = 6) out vec3 Emission;


layout(location = 0) in vec4 vertex;
layout(location = 1) in vec2 uv;
layout(location = 2) in vec3 normal;
layout(location = 3) in vec3 tangent;
layout(location = 4) in vec3 bitangent;

#include"scene.glsl"

void main() {

	const material mat = getMaterial();
	const global_rendering_settings glob_settings = constantCommon.constant.globalSettings;

	/*	*/
	Diffuse = texture(DiffuseTexture, uv).rgba * mat.diffuseColor;
	Diffuse.a *= texture(AlphaMaskedTexture, uv).r;

	if(texture(AlphaMaskedTexture, uv).r * texture(DiffuseTexture, uv).a < 0.25){
		discard;
	}

	Roughness_Metalic.r = texture(RoughnessTexture, uv).r;
	Roughness_Metalic.g = texture(MetalicTexture, uv).r;

	Specular.r = texture(RoughnessTexture, uv).r;

	Emission.rgb += mat.emission.rgb * texture(EmissionTexture, uv).rgb;

	/*	*/
	WorldSpace = vec4(vertex.xyz, 1);
	TextureCoord = vec4(uv, 0, 0);

	/*	Convert normal map texture to a vector.	*/
	const vec3 NormalMap = (2.0 * texture(NormalTexture, uv).xyz) - vec3(1.0, 1.0, 1.0);
	/*	Compute the new normal vector on the specific surface normal.	*/
	Normal = vec4(normalize(mat3(tangent, bitangent, normal) * NormalMap), 1);
}
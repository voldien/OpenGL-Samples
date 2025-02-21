#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_include : enable
#extension GL_GOOGLE_include_directive : enable

layout(location = 0) out vec4 fragColor;

layout(location = 0) in vec3 in_WorldPosition;
layout(location = 1) in vec2 UV;
layout(location = 2) in vec3 normal;
layout(location = 3) in vec3 tangent;
layout(location = 4) in vec3 bitangent;

#include "fog_frag.glsl"
#include "terrain_base.glsl"

layout(binding = 1) uniform sampler2D DiffuseTexture;
layout(binding = 2) uniform sampler2D NormalTexture;
layout(binding = 6) uniform sampler2D IrradianceTexture;

void main() {

	/*	Convert normal map texture to a vector.	*/
	vec3 NormalMapBump = 2.0 * texture(NormalTexture, UV).xyz - vec3(1.0, 1.0, 1.0);
	/*	Scale non forward axis.	*/
	NormalMapBump.xy *= 10;
	/*	Compute the new normal vector on the specific surface normal.	*/
	const vec3 alteredNormal = normalize(mat3(tangent, bitangent, normal) * NormalMapBump);

	const vec3 viewDir = normalize(ubo.camera.position.xyz - in_WorldPosition);

	/*	*/
	vec4 lightColor =
		computeBlinnDirectional(ubo.directional, alteredNormal, viewDir, ubo.shininess.r, ubo.specularColor.rgb);

	/*	*/
	const vec2 irradiance_uv = inverse_equirectangular(normalize(alteredNormal));
	const vec4 irradiance_color = texture(IrradianceTexture, irradiance_uv).rgba;

	const vec4 ramp[4] = {vec4(0, 0.02, 0.05, 0.086), vec4(0.166239, 0.038374, 0.005713, 0.26), vec4(1, 1, 1, 0.527273),
						  vec4(0.601909, 0.678011, 1, 1.0)};

	const vec3 heatColor = ColorRampLinear(in_WorldPosition.y * 0.01, ramp, 4);

	const vec4 terrainColor = vec4(heatColor, 1);

	const vec4 color = terrainColor * ubo.diffuseColor;
	const vec4 lighting = (ubo.ambientColor * irradiance_color + lightColor);

	/*	*/
	fragColor = vec4(lighting.rgb, 1) * color;
	fragColor.a = 1;
}

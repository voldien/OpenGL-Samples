#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_include : enable
#extension GL_GOOGLE_include_directive : enable

layout(location = 0) out vec4 fragColor;

layout(location = 0) in vec3 in_WorldPosition;
layout(location = 1) in vec2 UV;
layout(location = 2) in vec3 normal;
layout(location = 3) in vec3 tangent;


#include "terrain_base.glsl"
#include "fog_frag.glsl"

layout(binding = 1) uniform sampler2D DiffuseTexture;
layout(binding = 2) uniform sampler2D NormalTexture;
layout(binding = 2) uniform sampler2D IrradianceTexture;

void main() {

	const vec3 alteredNormal = normal;
	const vec3 viewDir = normalize(ubo.camera.position.xyz - in_WorldPosition);

	/*	*/
	vec4 lightColor =
		computeBlinnDirectional(ubo.directional, alteredNormal, viewDir, ubo.shininess.r, ubo.specularColor.rgb);

	/*	*/
	const vec2 irradiance_uv = inverse_equirectangular(normalize(normal));
	const vec4 irradiance_color = texture(IrradianceTexture, irradiance_uv).rgba;

	//
	fragColor = (lightColor + ubo.ambientColor * irradiance_color) * ubo.diffuseColor;
	fragColor.a = 1;
	// fragColor = vec4(normal, 1);
}

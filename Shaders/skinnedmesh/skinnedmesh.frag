#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_explicit_attrib_location : enable
#extension GL_ARB_uniform_buffer_object : enable
#extension GL_ARB_shading_language_include : enable
#extension GL_GOOGLE_include_directive : enable

layout(location = 0) out vec4 fragColor;

layout(location = 0) in vec3 FragIN_position;
layout(location = 1) in vec2 FragIN_uv;
layout(location = 2) in vec3 FragIN_normal;
layout(location = 3) in vec3 FragIN_tangent;

#include "pbr.glsl"
#include "phongblinn.glsl"
#include "skinnedmesh_common.glsl"

void main() {

	const material mat = getMaterial();
	const global_rendering_settings glob_settings = constantCommon.constant.globalSettings;

	vec3 viewDir = normalize(getCamera().position.xyz - FragIN_position);

	/*	Compute normal.	*/
	const vec3 NewNormal = getNormalFromMap(NormalTexture, FragIN_uv, FragIN_position, FragIN_normal);

	/*	*/
	const vec4 lightColor =
		computeBlinnDirectional(ubo.directional, NewNormal, viewDir, mat.specular_roughness.a, mat.specular_roughness.rgb);

	/*	*/
	const vec2 irradiance_uv = inverse_equirectangular(normalize(NewNormal));
	const vec4 irradiance_color = vec4(1);// texture(IrradianceTexture, irradiance_uv).rgba;

	const vec4 color = texture(DiffuseTexture, FragIN_uv) * mat.diffuseColor;
	const vec4 lighting = (glob_settings.ambientColor * mat.ambientColor * irradiance_color + lightColor);

	fragColor = vec4(lighting.rgb, 1) * color;
	fragColor.a *= texture(AlphaMaskedTexture, FragIN_uv).r;
	fragColor *= mat.transparency.rgba;
	fragColor.rgb += mat.emission.rgb * texture(EmissionTexture, FragIN_uv).rgb;
	if (fragColor.a < 0.8) {
		discard;
	}
}
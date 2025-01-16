#version 460 core
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_include : enable
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_control_flow_attributes : enable

layout(early_fragment_tests) in;

layout(location = 0) in vec2 coord;
layout(location = 1) in vec3 color;
layout(location = 2) in vec3 normal;

layout(location = 0) out vec4 fragColor;

layout(binding = 10) uniform sampler2D IrradianceMap;

#include "fog_frag.glsl"
#include "marchinbcube_common.glsl"
#include "pbr.glsl"

void main() {

	const vec3 albedo = color;
	const vec3 V = vec3(0, 1, 0);
	const float roughness = 0.5;

	const vec3 F0 = albedo;

	vec3 kS = fresnelSchlickRoughness(max(dot(normal, V), 0.0), F0, roughness);
	vec3 kD = 1.0 - kS;

	const vec2 irradiance_uv = inverse_equirectangular(normalize(normal));
	const vec4 irradiance_color = texture(IrradianceMap, irradiance_uv);
	vec3 diffuse = irradiance_color.xyz * albedo;
	vec3 ambient = (kD * diffuse);

	/*	*/
	fragColor = vec4(ambient, 1);
	fragColor = blendFog(fragColor, ubo.fogSettings);
}

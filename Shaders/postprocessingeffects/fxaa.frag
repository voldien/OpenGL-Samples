#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_include : enable
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_shader_realtime_clock : enable

precision mediump float;
precision mediump int;

/*  */
layout(location = 1) out vec4 fragColor;
layout(location = 0) in vec2 screenUV;

/*  */
layout(set = 0, binding = 0) uniform sampler2D ColorTexture;

layout(push_constant) uniform Settings {
	layout(offset = 0) float span_max;
	layout(offset = 4) float reduce_min;
	layout(offset = 8) float reduce_mul;
}
settings;

#include "postprocessing_base.glsl"


void main() {
	vec4 color;

	const vec2 source_texel_size = 1.0 / textureSize(ColorTexture, 0);

	const vec2 v_rgbM = screenUV;

	/*	*/
	const vec2 v_rgbNW = screenUV + source_texel_size * vec2(-1, 1);
	const vec2 v_rgbNE = screenUV + source_texel_size * vec2(1, -1);
	const vec2 v_rgbSW = screenUV + source_texel_size * vec2(-1, -1);
	const vec2 v_rgbSE = screenUV + source_texel_size * vec2(1, 1);

	/*	*/
	const vec3 rgbNW = texture(ColorTexture, v_rgbNW).rgb;
	const vec3 rgbNE = texture(ColorTexture, v_rgbNE).rgb;
	const vec3 rgbSW = texture(ColorTexture, v_rgbSW).rgb;
	const vec3 rgbSE = texture(ColorTexture, v_rgbSE).rgb;
	const vec4 texColor = texture(ColorTexture, v_rgbM);
	const vec3 rgbM = texColor.xyz;

	vec3 luma = vec3(0.299, 0.587, 0.114);
	float lumaNW = dot(rgbNW, luma);
	float lumaNE = dot(rgbNE, luma);
	float lumaSW = dot(rgbSW, luma);
	float lumaSE = dot(rgbSE, luma);
	float lumaM = dot(rgbM, luma);

	/*	*/
	const float lumaMin = min(lumaM, min(min(lumaNW, lumaNE), min(lumaSW, lumaSE)));
	const float lumaMax = max(lumaM, max(max(lumaNW, lumaNE), max(lumaSW, lumaSE)));

	mediump vec2 dir;
	dir.x = -((lumaNW + lumaNE) - (lumaSW + lumaSE));
	dir.y = ((lumaNW + lumaSW) - (lumaNE + lumaSE));

	float dirReduce = max((lumaNW + lumaNE + lumaSW + lumaSE) * (0.25 * settings.reduce_mul), settings.reduce_min);

	float rcpDirMin = 1.0 / (min(abs(dir.x), abs(dir.y)) + dirReduce);
	dir = min(vec2(settings.span_max, settings.span_max),
			  max(vec2(-settings.span_max, -settings.span_max), dir * rcpDirMin)) *
		  source_texel_size;

	vec3 rgbA = 0.5 * (texture(ColorTexture, screenUV + dir * (1.0 / 3.0 - 0.5)).xyz +
					   texture(ColorTexture, screenUV + dir * (2.0 / 3.0 - 0.5)).xyz);

	vec3 rgbB = rgbA * 0.5 + 0.25 * (texture(ColorTexture, screenUV + dir * -0.5).xyz +
									 texture(ColorTexture, screenUV + dir * 0.5).xyz);

	float lumaB = dot(rgbB, luma);
	if ((lumaB < lumaMin) || (lumaB > lumaMax)) {
		color = vec4(rgbA, 1);
	} else {
		color = vec4(rgbB, 1);
	}

	fragColor = color;
}

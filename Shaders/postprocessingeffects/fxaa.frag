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

/*	Settings parameters.	*/
layout(push_constant) uniform Settings {
	layout(offset = 0) float span_max;
	layout(offset = 4) float reduce_min;
	layout(offset = 8) float reduce_mul;
}
settings;

#include "postprocessing_base.glsl"


vec4 fxaa(const in sampler2D texture, const in vec2 textureCoord){
	return vec4(0);
}

void main() {
	vec4 color;

	/*	*/
	const vec2 source_texel_size = 1.0 / textureSize(ColorTexture, 0);

	/*	*/
	const ivec2 v_rgbNW = ivec2(-1, 1);
	const ivec2 v_rgbNE = ivec2(1, -1);
	const ivec2 v_rgbSW = ivec2(-1, -1);
	const ivec2 v_rgbSE = ivec2(1, 1);

	/*	*/
	const vec3 rgbNW = textureOffset(ColorTexture, screenUV, v_rgbNW).rgb;
	const vec3 rgbNE = textureOffset(ColorTexture, screenUV, v_rgbNE).rgb;
	const vec3 rgbSW = textureOffset(ColorTexture, screenUV, v_rgbSW).rgb;
	const vec3 rgbSE = textureOffset(ColorTexture, screenUV, v_rgbSE).rgb;
	const vec4 texColor = texture(ColorTexture, screenUV);
	const vec3 rgbM = texColor.xyz;

	/*	Calculate lumen for all sample points.	*/
	const vec3 luma = vec3(0.299, 0.587, 0.114);
	float lumaNW = dot(rgbNW, luma);
	float lumaNE = dot(rgbNE, luma);
	float lumaSW = dot(rgbSW, luma);
	float lumaSE = dot(rgbSE, luma);
	float lumaM = dot(rgbM, luma);

	/*	Lumen Range.	*/
	const float lumaMin = min(lumaM, min(min(lumaNW, lumaNE), min(lumaSW, lumaSE)));
	const float lumaMax = max(lumaM, max(max(lumaNW, lumaNE), max(lumaSW, lumaSE)));

	/*	*/
	mediump vec2 dir;
	dir.x = -((lumaNW + lumaNE) - (lumaSW + lumaSE));
	dir.y = ((lumaNW + lumaSW) - (lumaNE + lumaSE));

	/*	*/
	const float dirReduce =
		max((lumaNW + lumaNE + lumaSW + lumaSE) * (0.25 * settings.reduce_mul), settings.reduce_min);

	/*	*/
	const float rcpDirMin = 1.0 / (min(abs(dir.x), abs(dir.y)) + dirReduce);
	dir = min(vec2(settings.span_max, settings.span_max),
			  max(vec2(-settings.span_max, -settings.span_max), dir * rcpDirMin)) *
		  source_texel_size;

	/*	*/
	const vec3 rgbA = 0.5 * (texture(ColorTexture, screenUV + dir * (1.0 / 3.0 - 0.5)).xyz +
					   texture(ColorTexture, screenUV + dir * (2.0 / 3.0 - 0.5)).xyz);

	/*	*/
	const vec3 rgbB = rgbA * 0.5 + 0.25 * (texture(ColorTexture, screenUV + dir * -0.5).xyz +
									 texture(ColorTexture, screenUV + dir * 0.5).xyz);

	/*	*/
	const float lumaB = dot(rgbB, luma);
	if ((lumaB < lumaMin) || (lumaB > lumaMax)) {
		color = vec4(rgbA, 1);
	} else {
		color = vec4(rgbB, 1);
	}

	fragColor = color;
}

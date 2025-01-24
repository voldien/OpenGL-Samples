#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_include : enable
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_shader_realtime_clock : enable

/*  */
layout(location = 0) out vec4 fragColor;
/*  */
layout(location = 0) in vec2 screenUV;
/*  */
layout(set = 0, binding = 0) uniform sampler2D ColorTexture;

layout(push_constant) uniform Settings { layout(offset = 0) float size; }
settings;

#include "postprocessing_base.glsl"

// Generates a dot matrix effect based on UV coordinates, dot size, and aspect ratio
float create_dot_matrix(const in vec2 screenUV, const in float dot_size, const in vec2 aspect_ratio) {

	float dot_matrix = length(fract(screenUV * dot_size * aspect_ratio) - vec2(0.5)) *
					   2.0;					  // Wraps UV coordinates, centers, forms a circle, and scales
	dot_matrix = (1.0 - dot_matrix) * 10.0;	  // Invert and enhance for a defined border
	dot_matrix = clamp(dot_matrix, 0.0, 1.0); // Ensure value stays within [0, 1]

	return dot_matrix;
}

void main() {

	const vec2 texSize = textureSize(ColorTexture, 0);
	const vec2 aspect_ratio = vec2(1.0, texSize.x / texSize.y); // Maintain 1:1 aspect ratio for dots
	const vec2 pixelated_screenUV = pixelate_screenUV(screenUV, settings.size, aspect_ratio);
	const float dot_matrix = create_dot_matrix(screenUV, settings.size, aspect_ratio);
	const vec4 color = vec4(0, 0, 0, 1);
	fragColor = mix(color, texture(ColorTexture, pixelated_screenUV), dot_matrix);
}

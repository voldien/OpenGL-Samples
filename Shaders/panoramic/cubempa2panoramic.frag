#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_include : enable
#extension GL_GOOGLE_include_directive : enable

/*  */
layout(location = 0) out vec4 fragColor;
/*  */
layout(location = 0) in vec2 screenUV;

layout(set = 0, binding = 0) uniform samplerCube textureCubeMap;

#include "common.glsl"

void main() {
	const vec3 direction = equirectangular(2 * screenUV - 1);
	fragColor = textureLod(textureCubeMap, direction, 0);
}
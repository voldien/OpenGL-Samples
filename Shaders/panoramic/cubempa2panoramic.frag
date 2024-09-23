#version 460
#extension GL_ARB_separate_shader_objects : enable
/*  */
layout(location = 0) out vec4 fragColor;
/*  */
layout(location = 0) in vec2 uv;

layout(binding = 0) uniform samplerCube textureCubeMap;

vec3 equirectangular(vec2 xy) {
	const vec2 tc = xy / vec2(2.0) - 0.5;
	const vec2 thetaphi =
		((tc * 2.0) - vec2(1.0)) * vec2(3.1415926535897932384626433832795, 1.5707963267948966192313216916398);
	const vec3 rayDirection =
		vec3(cos(thetaphi.y) * cos(thetaphi.x), sin(thetaphi.y), cos(thetaphi.y) * sin(thetaphi.x));
	return rayDirection;
}

void main() {

	const vec3 direction = equirectangular(uv * 2 - 1);

	fragColor = textureLod(textureCubeMap, direction, 0);
}
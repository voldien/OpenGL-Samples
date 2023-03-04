#version 460
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) out vec4 fragColor;
layout(location = 0) in vec3 vVertex;

layout(binding = 0) uniform sampler2D panorama;

layout(binding = 1) uniform UniformBufferBlock {
	mat4 proj;
	mat4 modelViewProjection;
	vec4 tintColor;
	float exposure;
}
ubo;

vec3 equirectangular(vec2 xy) {
	const vec2 tc = xy / vec2(2.0) - 0.5;
	const vec2 thetaphi =
		((tc * 2.0) - vec2(1.0)) * vec2(3.1415926535897932384626433832795, 1.5707963267948966192313216916398);
	const vec3 rayDirection =
		vec3(cos(thetaphi.y) * cos(thetaphi.x), sin(thetaphi.y), cos(thetaphi.y) * sin(thetaphi.x));
	return rayDirection;
}

vec2 inverse_equirectangular(vec3 direction) {
	const vec2 invAtan = vec2(0.1591, 0.3183);
	vec2 uv = vec2(atan(direction.z, direction.x), asin(direction.y));
	uv *= invAtan;
	uv += 0.5;
	return uv;
}

void main() {

	const vec2 uv = inverse_equirectangular(normalize(vVertex));

	fragColor = textureLod(panorama, uv, 0) * ubo.tintColor;
	fragColor = vec4(1.0) - exp(-fragColor * ubo.exposure);

	const float gamma = 2.2;
	fragColor = pow(fragColor, vec4(1.0 / gamma));
}
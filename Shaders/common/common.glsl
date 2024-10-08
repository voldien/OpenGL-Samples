#include "material.glsl"
#include "noise.glsl"
#include "fog.glsl"

vec4 bump(const in sampler2D BumpTexture, const in vec2 uv, const float dist) {

	const vec2 size = vec2(2.0, 0.0);
	const vec2 offset = 1.0 / textureSize(BumpTexture, 0);

	const vec2 offxy = vec2(offset.x, offset.y);
	const vec2 offzy = vec2(offset.x, offset.y);
	const vec2 offyx = vec2(offset.x, offset.y);
	const vec2 offyz = vec2(offset.x, offset.y);

	const float s11 = texture(BumpTexture, uv).x;
	const float s01 = texture(BumpTexture, uv + offxy).x;
	const float s21 = texture(BumpTexture, uv + offzy).x;
	const float s10 = texture(BumpTexture, uv + offyx).x;
	const float s12 = texture(BumpTexture, uv + offyz).x;
	vec3 va = vec3(size.x, size.y, s21 - s01);
	vec3 vb = vec3(size.y, size.x, s12 - s10);
	va = normalize(va);
	vb = normalize(vb);
	const vec4 bump = vec4(cross(va, vb) / 2 + 0.5, 1.0);

	return bump;
}



float computeLightContributionFactor(const in vec3 direction, const in vec3 normalInput) {
	return max(0.0, dot(-normalInput, direction));
}

float rand(const in float seed) { return fract(sin(seed) * 100000.0); }

float rand(const in vec2 co) { return fract(sin(dot(co.xy, vec2(12.9898, 78.233))) * 43758.5453); }

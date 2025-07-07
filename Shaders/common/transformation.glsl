#ifndef _COMMON_TRANSFORMATION_
#define _COMMON_TRANSFORMATION_ 1

#ifdef GL_FRAGMENT_SHADER
#endif

float getExpToLinear(const in float start, const in float end, const in float expValue) {
	return ((2.0f * start) / (end + start - expValue * (end - start)));
}

float get_depth_linear(const in sampler2D inDepthTexture, const in vec2 coords, const in float start,
					   const in float end) {
	const float depth = texture(inDepthTexture, coords).x;
	return getExpToLinear(start, end, depth);
}

vec3 world_to_view(const in mat4 view, const in vec3 x, bool is_position) {
	return (view * vec4(x, float(is_position))).xyz;
}

vec3 world_to_ndc(const mat4 viewProj, vec3 x, bool is_position) {
	vec4 ndc = viewProj * vec4(x, float(is_position));
	return ndc.xyz / ndc.w;
}

vec3 world_to_ndc(const in vec3 x, mat4 transform) {
	vec4 ndc = transform * vec4(x, 1.0f);
	return ndc.xyz / ndc.w;
}

vec3 view_to_ndc(const in mat4 proj, vec3 x, bool is_position) {
	const vec4 ndc = proj * vec4(x, float(is_position));
	return ndc.xyz / ndc.w;
}

vec4 view_to_world(const in mat4 viewInverse, const in vec3 x, const in bool is_position) {
	vec4 world_position = viewInverse * vec4(x, float(is_position));

	return world_position / world_position.w;
}

vec2 world_to_uv(mat4 viewProj, vec3 x, const in bool is_position) {
	vec4 uv = viewProj * vec4(x, float(is_position));
	return (uv.xy / uv.w) * vec2(0.5f, -0.5f) + 0.5f;
}

vec2 view_to_uv(mat4 proj, vec3 x, bool is_position) {
	const vec4 uv = proj * vec4(x, float(is_position));

	return (uv.xy / uv.w) * 0.5 + 0.5;
}

vec4 screen_to_ndc(const in vec2 coordinate, const in float fragmentDepth) {
	const vec4 ndc = vec4(coordinate.x * 2.0 - 1.0, coordinate.y * 2.0 - 1.0, fragmentDepth * 2.0 - 1.0, 1.0);
	return ndc;
}

vec2 ndc_to_uv(const in vec2 x) { return x * 0.5f + 0.5; }

vec3 ndc_to_uv(const in vec3 x) { return x.xyz * 0.5 + 0.5; }

vec4 ndc_to_uv(const in vec4 x) { return x * vec4(0.5, 0.5, 0.5, 1) + vec4(0.5, 0.5, 0.5, 0); }

vec4 ndc_to_view(const in mat4 inversProj, const in vec4 ndc) {
	vec4 vs_pos = inversProj * ndc;
	return vs_pos.xyzw / vs_pos.w;
}

vec3 calcViewPosition(const in vec2 coords, const in mat4 inverseProj, const in float fragmentDepth) {

	/*	Convert from screenspace to Normalize Device Coordinate.	*/
	const vec4 ndc = vec4(coords.x * 2.0 - 1.0, coords.y * 2.0 - 1.0, fragmentDepth * 2.0 - 1.0, 1.0);

	/*	Transform to view space using inverse camera projection matrix.	*/
	vec4 vs_pos = inverseProj * ndc;

	/*	*/
	vs_pos.xyz = vs_pos.xyz / vs_pos.w;

	return vs_pos.xyz;
}

#endif
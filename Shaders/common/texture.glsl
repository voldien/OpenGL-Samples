#ifndef _COMMON_TEXTURE_H_
#define _COMMON_TEXTURE_H_ 1

vec4 bump(const in sampler2D BumpTexture, const in vec2 uv, const in float dist) {

	/*	*/
	const vec2 size = vec2(2.0, 0.0);
	const vec2 offset = 1.0 / textureSize(BumpTexture, 0);

	/*	*/
	const vec2 offxy = vec2(offset.x, offset.y);
	const vec2 offzy = vec2(-offset.x, offset.y);
	const vec2 offyx = vec2(offset.x, -offset.y);
	const vec2 offyz = vec2(-offset.x, -offset.y);

	const float bump_strength = dist;

	/*	*/
	const float s11 = texture(BumpTexture, uv).x;
	const float s01 = texture(BumpTexture, uv + offxy).x;
	const float s21 = texture(BumpTexture, uv + offzy).x;
	const float s10 = texture(BumpTexture, uv + offyx).x;
	const float s12 = texture(BumpTexture, uv + offyz).x;

	vec3 va = bump_strength * vec3(size.x, size.y, s21 - s10);
	vec3 vb = bump_strength * vec3(size.y, size.x, s12 - s01);

	va = normalize(va);
	vb = normalize(vb);

	const vec4 normal = vec4(cross(va, vb) * 0.5 + 0.5, 1.0);

	return normal;
}

vec3 sobel(const sampler2D color_texture, const float source_radius, const vec2 normalized_coordinate) {

	const vec2 sourceSize = textureSize(color_texture, 0);
	const vec2 source_texel_size = (1.0 / sourceSize);
	const vec2 sample_offset = source_texel_size * source_radius;

	vec2 sampleTexCoord = normalized_coordinate;

	// TODO add recompute of the sample kernel coordinates.
	const vec2 lb = vec2(-1 * sample_offset.x, -1 * sample_offset.y);
	const vec4 p00 = texture(color_texture, sampleTexCoord + lb);
	const vec2 b = vec2(0.0 * sample_offset.x, -1 * sample_offset.y);
	const vec4 p10 = texture(color_texture, sampleTexCoord + b);
	const vec2 rb = vec2(1 * sample_offset.x, -1 * sample_offset.y);
	const vec4 p20 = texture(color_texture, sampleTexCoord + rb);

	const vec2 l = vec2(-1 * sample_offset.x, 0.0 * sample_offset.y);
	const vec4 p01 = texture(color_texture, sampleTexCoord + l);
	const vec2 r = vec2(1 * sample_offset.x, 0.0 * sample_offset.y);
	const vec4 p21 = texture(color_texture, sampleTexCoord + r);

	const vec2 lt = vec2(-1 * sample_offset.x, 1 * sample_offset.y);
	const vec4 p02 = texture(color_texture, sampleTexCoord + lt);
	const vec2 t = vec2(-1 * sample_offset.x, 1 * sample_offset.y);
	const vec4 p12 = texture(color_texture, sampleTexCoord + t);
	const vec2 rt = vec2(1 * sample_offset.x, 1 * sample_offset.y);
	const vec4 p22 = texture(color_texture, sampleTexCoord + rt);

	/* Compute Matrix X and Y.	*/
	const vec3 gx = -p00.xyz + p20.xyz + 2.0 * (p21.xyz - p01.xyz) - p02.xyz + p22.xyz;
	const vec3 gy = -p00.xyz - p20.xyz + 2.0 * (p12.xyz - p10.xyz) + p02.xyz + p22.xyz;

	/* Compute the final.	*/
	const vec3 edgeColor = sqrt(gx * gx + gy * gy);

	return edgeColor;
	// float prop = step(_ThresHold, length(round(g.rgb)));
	// float4 edge = lerp(color.rgba, _Color.rgba, prop);
	// g = edge.rgb;
	// g = 1 - (1 - color.rgb) * (1 - g.rgb * _Color * length(g));
}

vec4 bump(const in float height, const in float dist) {

	const float x = 0; // dFdx(height) * dist;
	const float y = 0; // dFdy(height) * dist;

	const vec4 normal = vec4(x, y, 1, 0);

	return normalize(normal);
}

#endif
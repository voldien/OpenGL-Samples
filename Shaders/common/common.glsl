

vec4 bump(const in sampler2D BumpTexture, const in vec2 uv, const float dist) {

	const vec2 size = vec2(2.0, 0.0);
	const vec2 offset = 1.0 / textureSize(BumpTexture, 0);

	vec2 offxy = vec2(offset.x, offset.y);
	vec2 offzy = vec2(offset.x, offset.y);
	vec2 offyx = vec2(offset.x, offset.y);
	vec2 offyz = vec2(offset.x, offset.y);

	float s11 = texture(BumpTexture, uv).x;
	float s01 = texture(BumpTexture, uv + offxy).x;
	float s21 = texture(BumpTexture, uv + offzy).x;
	float s10 = texture(BumpTexture, uv + offyx).x;
	float s12 = texture(BumpTexture, uv + offyz).x;
	vec3 va = vec3(size.x, size.y, s21 - s01);
	vec3 vb = vec3(size.y, size.x, s12 - s10);
	va = normalize(va);
	vb = normalize(vb);
	vec4 bump = vec4(cross(va, vb) / 2 + 0.5, 1.0);

	return bump;
}

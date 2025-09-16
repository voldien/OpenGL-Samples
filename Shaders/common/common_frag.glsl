#ifndef _COMMON_FRAG_H_
#define _COMMON_FRAG_H_ 1

/*	Fragment stage extract normal,*/
vec3 getNormalFromMap(const in sampler2D normalMap, const in vec2 TexCoords, const in vec3 WorldPos,
					   const in vec3 Normal, const float bumpiness) {
	/*	*/
	vec3 tangentNormal = texture(normalMap, TexCoords).xyz * 2.0 - 1.0;
	tangentNormal.xy *= bumpiness;

	const vec3 Q1 = dFdx(WorldPos);
	const vec3 Q2 = dFdy(WorldPos);
	const vec2 st1 = dFdx(TexCoords);
	const vec2 st2 = dFdy(TexCoords);

	const vec3 N = normalize(Normal);
	const vec3 T = normalize(Q1 * st2.t - Q2 * st1.t);
	const vec3 B = -normalize(cross(N, T));
	const mat3 TBN = mat3(T, B, N);

	return normalize(TBN * tangentNormal);
}

void discardMethod(const in float alpha, const in float clip){
	if(alpha < clip){
		discard;
	}
}

#endif
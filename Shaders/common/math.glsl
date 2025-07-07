#ifndef _COMMON_MATH_H_
#define _COMMON_MATH_H_ 1

/*	Constants.	*/
#define PI 3.1415926535897932384626433832795
#define PI_HALF (PI / 2.0)
#define E_CONSTANT 2.7182818284590

float map(float s, float a1, float a2, float b1, float b2)
{
    return b1 + (s-a1)*(b2-b1)/(a2-a1);
}

// Rayleigh phase function
vec3 rayleigh_phase(vec3 L, const in vec3 V, const float betaR) {
	return vec3(3.0 / (16.0 * PI)) * betaR * (1.0 + (pow(dot(L, V), 2)));
}

// Mie phase function (Henyey-Greenstein approximation)
float mie_phase(const in float cosTheta, const in float g, const float betaM) {
	float divisor = pow(1.0 - (2.0 * g * cosTheta) + g * g, 1.5);
	return (betaM / (4.0 * PI)) * ((1.0 + (pow(cosTheta, 2))) / divisor);
}

#endif
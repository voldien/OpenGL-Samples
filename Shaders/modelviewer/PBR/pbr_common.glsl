#include "common.glsl"
#include "light.glsl"
#include "scene.glsl"
#include "shadow.glsl"

/*	Physical Based Rendering.	*/
layout(constant_id = 32) const bool UseImageBasedLightning = true;	//TODO: as number, for adding flags.
layout(constant_id = 33) const int DistrubtionFunction = 1;

struct OutputPatch {
	vec3 WorldPos_B030;
	vec3 WorldPos_B021;
	vec3 WorldPos_B012;
	vec3 WorldPos_B003;
	vec3 WorldPos_B102;
	vec3 WorldPos_B201;
	vec3 WorldPos_B300;
	vec3 WorldPos_B210;
	vec3 WorldPos_B120;
	vec3 WorldPos_B111;
	vec3 Normal[3];
	vec3 Tormal[3];
	vec2 TexCoord[3];
};
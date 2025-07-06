#include "common.glsl"
#include "light.glsl"
#include "scene.glsl"

layout(binding = 0, std140) uniform UniformBufferBlock {
 
	tessellation_settings tessellation;
}
ubo;

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
#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_include : enable
#extension GL_GOOGLE_include_directive : enable

layout(triangles, equal_spacing, ccw) in;

layout(location = 0) out vec4 LightColor_FS_in;

#include "gouraud_base.glsl"

layout(location = 0) in vec3 WorldPos_ES_in[];
layout(location = 1) in vec3 Normal_ES_in[];

vec2 interpolate2D(vec2 v0, vec2 v1, vec2 v2) {
	return vec2(gl_TessCoord.x) * v0 + vec2(gl_TessCoord.y) * v1 + vec2(gl_TessCoord.z) * v2;
}

vec3 interpolate3D(vec3 v0, vec3 v1, vec3 v2) {
	return vec3(gl_TessCoord.x) * v0 + vec3(gl_TessCoord.y) * v1 + vec3(gl_TessCoord.z) * v2;
}

float computeLightContributionFactor(in vec3 direction, in vec3 normalInput) {
	return max(0.0, dot(-normalInput, direction));
}

void main() {

	const vec3 Normal_FS_in = normalize(interpolate3D(Normal_ES_in[0], Normal_ES_in[1], Normal_ES_in[2]));
	const vec3 WorldPos_FS_in = interpolate3D(WorldPos_ES_in[0], WorldPos_ES_in[1], WorldPos_ES_in[2]);

	LightColor_FS_in =
		ubo.diffuseColor * computeLightContributionFactor(ubo.direction.xyz, Normal_FS_in) * ubo.lightColor +
		ubo.ambientColor;

	gl_Position = ubo.viewProjection * vec4(WorldPos_FS_in, 1.0);
}

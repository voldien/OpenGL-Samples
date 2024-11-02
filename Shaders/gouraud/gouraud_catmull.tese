#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_include : enable
#extension GL_GOOGLE_include_directive : enable

layout(triangles, equal_spacing, ccw) in;

layout(location = 0) out vec4 LightColor_FS_in;

#include "gouraud_base.glsl"

layout(location = 4) in patch OutputPatch oPatch;

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
	// Interpolate the attributes of the output vertex using the barycentric coordinates
	// TexCoord_FS_in = interpolate2D(oPatch.TexCoord[0], oPatch.TexCoord[1], oPatch.TexCoord[2]);
	const vec3 Normal_FS_in = normalize(interpolate3D(oPatch.Normal[0], oPatch.Normal[1], oPatch.Normal[2]));

	LightColor_FS_in =
		ubo.diffuseColor * computeLightContributionFactor(ubo.direction.xyz, Normal_FS_in) * ubo.lightColor +
		ubo.ambientColor;

	const float u = gl_TessCoord.x;
	const float v = gl_TessCoord.y;
	const float w = gl_TessCoord.z;

	const float uPow3 = pow(u, 3);
	const float vPow3 = pow(v, 3);
	const float wPow3 = pow(w, 3);
	const float uPow2 = pow(u, 2);
	const float vPow2 = pow(v, 2);
	const float wPow2 = pow(w, 2);

	const vec3 WorldPos_FS_in = oPatch.WorldPos_B300 * wPow3 + oPatch.WorldPos_B030 * uPow3 +
								oPatch.WorldPos_B003 * vPow3 + oPatch.WorldPos_B210 * 3.0 * wPow2 * u +
								oPatch.WorldPos_B120 * 3.0 * w * uPow2 + oPatch.WorldPos_B201 * 3.0 * wPow2 * v +
								oPatch.WorldPos_B021 * 3.0 * uPow2 * v + oPatch.WorldPos_B102 * 3.0 * w * vPow2 +
								oPatch.WorldPos_B012 * 3.0 * u * vPow2 + oPatch.WorldPos_B111 * 6.0 * w * u * v;

	gl_Position = ubo.viewProjection * vec4(WorldPos_FS_in, 1.0);
}

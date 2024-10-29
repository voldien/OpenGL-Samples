#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_include : enable
#extension GL_GOOGLE_include_directive : enable

layout(triangles, equal_spacing, ccw) in;

layout(location = 0) out vec3 WorldPos_FS_in;
layout(location = 1) out vec2 TexCoord_FS_in;
layout(location = 2) out vec3 Normal_FS_in;
layout(location = 3) out vec3 Tangent_FS_in;

#include "common.glsl"
#include "terrain_base.glsl"

layout(location = 4) in patch OutputPatch oPatch;

layout(binding = 2) uniform sampler2D gDisplacementMap;

vec2 interpolate2D(vec2 v0, vec2 v1, vec2 v2) {
	return vec2(gl_TessCoord.x) * v0 + vec2(gl_TessCoord.y) * v1 + vec2(gl_TessCoord.z) * v2;
}

vec3 interpolate3D(vec3 v0, vec3 v1, vec3 v2) {
	return vec3(gl_TessCoord.x) * v0 + vec3(gl_TessCoord.y) * v1 + vec3(gl_TessCoord.z) * v2;
}

void main() {
	// Interpolate the attributes of the output vertex using the barycentric coordinates
	TexCoord_FS_in = interpolate2D(oPatch.TexCoord[0], oPatch.TexCoord[1], oPatch.TexCoord[2]);
	Normal_FS_in = normalize(interpolate3D(oPatch.Normal[0], oPatch.Normal[1], oPatch.Normal[2]));
	/*	Tangent.	*/
	Tangent_FS_in = normalize(interpolate3D(oPatch.Tangent[0], oPatch.Tangent[1], oPatch.Tangent[2]));

	const float u = gl_TessCoord.x;
	const float v = gl_TessCoord.y;
	const float w = gl_TessCoord.z;

	const float uPow3 = pow(u, 3);
	const float vPow3 = pow(v, 3);
	const float wPow3 = pow(w, 3);
	const float uPow2 = pow(u, 2);
	const float vPow2 = pow(v, 2);
	const float wPow2 = pow(w, 2);

	WorldPos_FS_in = oPatch.WorldPos_B300 * wPow3 + oPatch.WorldPos_B030 * uPow3 + oPatch.WorldPos_B003 * vPow3 +
					 oPatch.WorldPos_B210 * 3.0 * wPow2 * u + oPatch.WorldPos_B120 * 3.0 * w * uPow2 +
					 oPatch.WorldPos_B201 * 3.0 * wPow2 * v + oPatch.WorldPos_B021 * 3.0 * uPow2 * v +
					 oPatch.WorldPos_B102 * 3.0 * w * vPow2 + oPatch.WorldPos_B012 * 3.0 * u * vPow2 +
					 oPatch.WorldPos_B111 * 6.0 * w * u * v;

	/*	Displace the vertex along the normal	*/
	const float heightMapDisp = 0;						// texture(gDisplacementMap, TexCoord_FS_in.xy).r;
	WorldPos_FS_in += Normal_FS_in * heightMapDisp * 1; // ubo.gDispFactor;

	// vec3 Mnormal = normalize(FragIN_normal);
	// vec3 Ttangent = normalize(FragIN_tangent);

	// Ttangent = normalize(Ttangent - dot(Ttangent, Mnormal) * Mnormal);
	// FragIN_bitangent = cross(Ttangent, Mnormal);

	gl_Position = ubo.viewProjection * vec4(WorldPos_FS_in, 1.0);
}

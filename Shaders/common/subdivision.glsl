#ifndef SUBDIVISION_H
#define SUBDIVISION_H
#include"common.glsl"

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
	vec2 TexCoord[3];
};

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


void CalcPositions(OutputPatch oPatch) {
	// The original vertices stay the same
	oPatch.WorldPos_B030 = WorldPos_CS_in[0];
	oPatch.WorldPos_B003 = WorldPos_CS_in[1];
	oPatch.WorldPos_B300 = WorldPos_CS_in[2];

	// Edges are names according to the opposing vertex
	vec3 EdgeB300 = oPatch.WorldPos_B003 - oPatch.WorldPos_B030;
	vec3 EdgeB030 = oPatch.WorldPos_B300 - oPatch.WorldPos_B003;
	vec3 EdgeB003 = oPatch.WorldPos_B030 - oPatch.WorldPos_B300;

	// Generate two midpoints on each edge
	oPatch.WorldPos_B021 = oPatch.WorldPos_B030 + EdgeB300 / 3.0;
	oPatch.WorldPos_B012 = oPatch.WorldPos_B030 + EdgeB300 * 2.0 / 3.0;
	oPatch.WorldPos_B102 = oPatch.WorldPos_B003 + EdgeB030 / 3.0;
	oPatch.WorldPos_B201 = oPatch.WorldPos_B003 + EdgeB030 * 2.0 / 3.0;
	oPatch.WorldPos_B210 = oPatch.WorldPos_B300 + EdgeB003 / 3.0;
	oPatch.WorldPos_B120 = oPatch.WorldPos_B300 + EdgeB003 * 2.0 / 3.0;

	// Project each midpoint on the plane defined by the nearest vertex and its normal
	oPatch.WorldPos_B021 = ProjectToPlane(oPatch.WorldPos_B021, oPatch.WorldPos_B030, oPatch.Normal[0]);
	oPatch.WorldPos_B012 = ProjectToPlane(oPatch.WorldPos_B012, oPatch.WorldPos_B003, oPatch.Normal[1]);
	oPatch.WorldPos_B102 = ProjectToPlane(oPatch.WorldPos_B102, oPatch.WorldPos_B003, oPatch.Normal[1]);
	oPatch.WorldPos_B201 = ProjectToPlane(oPatch.WorldPos_B201, oPatch.WorldPos_B300, oPatch.Normal[2]);
	oPatch.WorldPos_B210 = ProjectToPlane(oPatch.WorldPos_B210, oPatch.WorldPos_B300, oPatch.Normal[2]);
	oPatch.WorldPos_B120 = ProjectToPlane(oPatch.WorldPos_B120, oPatch.WorldPos_B030, oPatch.Normal[0]);

	// Handle the center
	vec3 Center = (oPatch.WorldPos_B003 + oPatch.WorldPos_B030 + oPatch.WorldPos_B300) / 3.0;
	oPatch.WorldPos_B111 = (oPatch.WorldPos_B021 + oPatch.WorldPos_B012 + oPatch.WorldPos_B102 + oPatch.WorldPos_B201 +
							oPatch.WorldPos_B210 + oPatch.WorldPos_B120) /
						   6.0;
	oPatch.WorldPos_B111 += (oPatch.WorldPos_B111 - Center) / 2.0;
}


#endif
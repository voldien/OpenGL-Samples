#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_include : enable
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_control_flow_attributes : enable

// define the number of CPs in the output patch
layout(vertices = 1) out;

layout(location = 0) in vec3 WorldPos_CS_in[];
layout(location = 1) in vec3 Normal_CS_in[];

#include "gouraud_base.glsl"

layout(location = 4) out patch OutputPatch oPatch;

vec3 ProjectToPlane(const in vec3 Point, const in vec3 PlanePoint, const in vec3 PlaneNormal) {
	const vec3 v = Point - PlanePoint;
	const float Len = dot(v, PlaneNormal);
	const vec3 d = Len * PlaneNormal;
	return (Point - d);
}

void CalcPositions() {
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

float GetTessLevel(float Distance0, float Distance1) {
	float AvgDistance = (Distance0 + Distance1) / 2.0;

	const float maxTessellation = 20.0;
	const float minTessellation = 0.04;

	return mix(minTessellation, maxTessellation, 100 / (AvgDistance + 10));
}

void main() {

	// Set the control points of the output patch
	[[unroll]] for (uint i = 0; i < 3; i++) {
		oPatch.Normal[i] = Normal_CS_in[i];
	}

	CalcPositions();

	/*	Calculate the distance from the camera to the three control points	*/
	float EyeToVertexDistance0 = distance(ubo.gEyeWorldPos.xyz, (ubo.view * vec4(oPatch.WorldPos_B030, 1)).xyz);
	float EyeToVertexDistance1 = distance(ubo.gEyeWorldPos.xyz, (ubo.view * vec4(oPatch.WorldPos_B021, 1)).xyz);
	float EyeToVertexDistance2 = distance(ubo.gEyeWorldPos.xyz, (ubo.view * vec4(oPatch.WorldPos_B201, 1)).xyz);

	/*	Calculate the tessellation levels	*/
	gl_TessLevelOuter[0] = GetTessLevel(EyeToVertexDistance1, EyeToVertexDistance2) * ubo.tessLevel;
	gl_TessLevelOuter[1] = GetTessLevel(EyeToVertexDistance2, EyeToVertexDistance0) * ubo.tessLevel;
	gl_TessLevelOuter[2] = GetTessLevel(EyeToVertexDistance0, EyeToVertexDistance1) * ubo.tessLevel;
	gl_TessLevelInner[0] = gl_TessLevelOuter[2];

	gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;
}
